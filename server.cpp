#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cassert>
// system headers
#include <unistd.h>
#if defined(WIN32) || defined(_WIN32)
  #include <winsock2.h>
#else
  #include <sys/types.h>
#endif
// libraries headers
#include "uv.h"

class Server_Parameters {
public:
  Server_Parameters() {}

  Server_Parameters(int argc, char *argv[], std::string&& s) {
    char option = '\0';
    constexpr int getopt_finished = -1;
    
    // switch off getopt error messages
    opterr = 0;
    // according to the task we have to process -h, -p, -d flags
    // 'h': server ip
    // 'p': server port
    // 'd': server dir
    while ((option = getopt(argc, argv, s.c_str())) != getopt_finished) {
      switch (option) {
        case 'h':
          ip = optarg;
          break;
        case 'p':
          port = std::stoi(optarg);
          break;
        case 'd':
          /* TODO: add check for directory availability */
          directory = optarg;
          break;
        default:
          /* TODO: handle wrong arguments somehow */
          break;
      }
    }
  }
  
  // bind to all avaliable interfaces
  std::string ip{"0.0.0.0"};
  // default port
  int port{8000};
  // use server executable current directory
  std::string directory{"."};
};

class HTTP_Client {
public:
  HTTP_Client() {}
  
private:
  uv_tcp_t client_handle;
  
  // we need HTTP server to have access to client handle for initializing it
  friend class HTTP_Server;
};

class HTTP_Server {
public:
  HTTP_Server(uv_loop_t * const loop, const Server_Parameters& sp) {
    assert(loop != nullptr);
    struct sockaddr_in sin;
    constexpr int keep_alive_delay = 60;
    int status = uv_ip4_addr(sp.ip.c_str(), sp.port, &sin);
    
    assert(status == 0);
    status = uv_tcp_init(loop, &server_handle);
    assert(status == 0);
    status = uv_tcp_keepalive(&server_handle, true, keep_alive_delay);
    assert(status == 0);
    status = uv_tcp_bind(&server_handle, reinterpret_cast<struct sockaddr *> (&sin), 0);
    assert(status == 0);
    status = uv_listen(reinterpret_cast<uv_stream_t *> (&server_handle), SOMAXCONN, on_connect);
    assert(status == 0);
  }
  // prohibit copying
  HTTP_Server(const HTTP_Server& serv) = delete;
  HTTP_Server& operator=(const HTTP_Server& serv) = delete;
private:
  uv_loop_t *loop;
  uv_tcp_t server_handle;
  std::string server_directory;
  
  static void on_connect(uv_stream_t *server, int status) {
    /* TODO: accept client connection */
    assert(status == 0);
    HTTP_Client *incoming_conn = new HTTP_Client;
    
    uv_tcp_t *client_stream = reinterpret_cast<uv_tcp_t *> (&incoming_conn->client_handle);
    status = uv_tcp_init(server->loop, client_stream);
    assert(status == 0);
    client_stream->data = incoming_conn;
    status = uv_accept(server, reinterpret_cast<uv_stream_t *> (client_stream));
    assert(status == 0);
    status = uv_read_start(reinterpret_cast<uv_stream_t *> (client_stream), alloc_buffer, on_read);
  }
  
  static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf) {
    *buf = uv_buf_init(new char[suggested_size], suggested_size);
  }
  
  static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread >= 0) {
      // get HTTP request and parse it
      std::cout << buf->base << std::endl;
    }
    else {
      // closed session or error happened
      uv_shutdown_t *shutdown_req = new uv_shutdown_t;
      
      uv_shutdown(shutdown_req, client, on_shutdown);
    }
    delete buf->base;
  }
  
  static void on_shutdown(uv_shutdown_t *req, int status) {
    assert(status == 0);
    uv_close(reinterpret_cast<uv_handle_t *> (req->handle), on_close);
    delete req;
  }
  
  static void on_close(uv_handle_t *handle) {
    HTTP_Client *client = reinterpret_cast<HTTP_Client *> (handle->data);
    
    delete client;
  }
};

int main(int argc, char *argv[]) {
  uv_loop_t *loop = uv_default_loop();
  Server_Parameters sp(argc, argv, "h:p:d:");
  HTTP_Server(loop, sp);
  
  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);
  
  
  return 0;
}