#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cassert>
#include <vector>
#include <iterator>
#include <iomanip>
// system headers
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#if defined(WIN32) || defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/types.h>
#endif
// libraries headers
#include "uv.h"

constexpr auto HTTP_200 = "200 OK";
constexpr auto HTTP_404 = "404 NOT FOUND";
constexpr auto Connection_Close = "Connection: close";
constexpr auto Content_Length = "Content-Length: ";
constexpr auto MIME_text_html = "Content-Type: text/html; charset=utf-8";

class Server_Parameters {
 public:
  Server_Parameters() {}

  Server_Parameters(int argc, char *argv[], std::string &&s) {
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
  HTTP_Client(const std::string &rd) : root_directory{rd} {}

  const std::string &root_directory;

 private:
  uv_tcp_t client_handle;

  // we need HTTP server to have access to client handle for initializing it
  friend class HTTP_Server;
};

class HTTP_Server {
 private:
  struct Parser_Req {
    Parser_Req(const std::string &rd, char *d, uv_stream_t *cs)
        : root_dir{rd}, req_data{d}, client_s{cs} {}

    ~Parser_Req() {
      // release memory which was alloceted in uv_buf_t after we finish parsing
      // process
      delete req_data;
    }
    // now there is only GET requests handling
    enum class request_fields {
      REQ_METHOD = 0,
      REQ_PATH = 1,
      REQ_QUERY = 2,
      REQ_PROTO = 3
    };

    static void parse_request(uv_work_t *req) {
      std::string method;
      std::string path;
      std::string protocol;
      Parser_Req *parser_ptr = static_cast<Parser_Req *>(req->data);
      std::istringstream iss{parser_ptr->req_data};

      // get request type
      iss >> method;

      if (method == "GET") {
        /* TODO: Handle GET query */
        iss >> path;
        // remove query part from the path
        get_query(path);
        // get full path to a requested directory
        auto full_path = parser_ptr->root_dir + path;
        iss >> protocol;

        parser_ptr->response = render_get(req->loop, full_path, protocol);
      } else {
        /* TODO: Handle all other methods */
        assert(0 && "Request type is not supported yet");
      }
    }

    static std::string get_query(std::string &path) {
      constexpr char query_break = '?';
      std::size_t path_offset = path.find_first_of(query_break);
      std::string query;

      if (path_offset != std::string::npos) {
        query = path.substr(path_offset);
        // erase query part from the path
        path.erase(path.begin() + path_offset, path.end());
      }

      return query;
    }

    static std::string render_get(uv_loop_t *loop, const std::string &path,
                                  const std::string &protocol) {
      uv_fs_t scan_path;
      int status = uv_fs_stat(loop, &scan_path, path.c_str(), nullptr);
      std::ostringstream oss;
      std::string response;

      // error happened or requested URI is for directory
      if (scan_path.result || S_ISDIR(scan_path.statbuf.st_mode)) {
        oss << protocol << " " << HTTP_404 << std::endl
            << MIME_text_html << std::endl;
        oss << Connection_Close << std::endl << std::endl;
      } else {
        std::ifstream ifs{path};
        // we have to save original punctuation and spacing :)
        ifs >> std::noskipws;
        std::string s{std::istream_iterator<char>{ifs},
                      std::istream_iterator<char>{}};
        oss << protocol << " " << HTTP_200 << std::endl
            << MIME_text_html << std::endl;
        oss << Connection_Close << std::endl
            << Content_Length << s.size() << std::endl
            << std::endl;
        oss << std::noskipws << s;
      }

      response = oss.str();
      uv_fs_req_cleanup(&scan_path);

      return response;
    }

    // HTTP server root dir
    const std::string &root_dir;
    // pointer to data obtained from client
    char *req_data;
    // pointer to a client stream
    uv_stream_t *client_s;
    // response string
    std::string response;
  };

 public:
  HTTP_Server(uv_loop_t *const loop, const Server_Parameters &sp)
      : server_directory{sp.directory} {
    assert(loop != nullptr);
    struct sockaddr_in sin;
    constexpr int keep_alive_delay = 60;
    int status = uv_ip4_addr(sp.ip.c_str(), sp.port, &sin);

    assert(status == 0 &&
           "Cannot convert incomin IP and port into sockaddr_in");
    status = uv_tcp_init(loop, &server_handle);
    assert(status == 0);
    status = uv_tcp_keepalive(&server_handle, true, keep_alive_delay);
    assert(status == 0);
    status = uv_tcp_bind(&server_handle,
                         reinterpret_cast<struct sockaddr *>(&sin), 0);
    assert(status == 0);
    server_handle.data = &server_directory;
    status = uv_listen(reinterpret_cast<uv_stream_t *>(&server_handle),
                       SOMAXCONN, on_connect);
    assert(status == 0);
  }
  // prohibit copying
  HTTP_Server(const HTTP_Server &serv) = delete;
  HTTP_Server &operator=(const HTTP_Server &serv) = delete;

 private:
  uv_tcp_t server_handle;
  std::string server_directory;

  static void on_connect(uv_stream_t *server, int status) {
    /* TODO: accept client connection */
    assert(status == 0);
    std::string *s_ptr = static_cast<std::string *>(server->data);
    HTTP_Client *incoming_conn = new HTTP_Client(*s_ptr);

    uv_tcp_t *client_stream =
        reinterpret_cast<uv_tcp_t *>(&incoming_conn->client_handle);
    status = uv_tcp_init(server->loop, client_stream);
    assert(status == 0);
    client_stream->data = incoming_conn;
    status = uv_accept(server, reinterpret_cast<uv_stream_t *>(client_stream));
    assert(status == 0 && "Cannot accept incoming connection");
    status = uv_read_start(reinterpret_cast<uv_stream_t *>(client_stream),
                           alloc_buffer, on_read);
    assert(status == 0 && "Cannot start reading");
  }

  static void alloc_buffer(uv_handle_t *handle, size_t suggested_size,
                           uv_buf_t *buf) {
    *buf = uv_buf_init(new char[suggested_size], suggested_size);
  }

  static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread >= 0) {
      // get HTTP request and parse it
      HTTP_Client *client_ptr = static_cast<HTTP_Client *>(client->data);
      uv_work_t *parse_work = new uv_work_t;

      parse_work->data =
          new Parser_Req(client_ptr->root_directory, buf->base, client);
      int status = uv_queue_work(client->loop, parse_work,
                                 Parser_Req::parse_request, send_response);
      assert(status == 0);
    } else {
      // closed session or error happened
      send_shutdown(client);
      delete buf->base;
    }
  }

  static void on_shutdown(uv_shutdown_t *req, int status) {
    assert(status == 0);
    uv_close(reinterpret_cast<uv_handle_t *>(req->handle), on_close);
    delete req;
  }

  static void on_close(uv_handle_t *handle) {
    HTTP_Client *client = reinterpret_cast<HTTP_Client *>(handle->data);

    delete client;
  }

  static void send_response(uv_work_t *req, int status) {
    Parser_Req *pr = static_cast<Parser_Req *>(req->data);
    uv_write_t *wr = new uv_write_t;
    uv_buf_t buf = uv_buf_init(const_cast<char *>(pr->response.c_str()),
                               pr->response.size());

    uv_write(wr, pr->client_s, &buf, 1, on_write);
    delete pr;
  }

  static void on_write(uv_write_t *req, int status) {
    assert(status == 0);
    uv_handle_t *client_s = reinterpret_cast<uv_handle_t *>(req->handle);

    if (!uv_is_closing(client_s)) {
      send_shutdown(reinterpret_cast<uv_stream_t *>(client_s));
    }

    delete req;
  }

  static void send_shutdown(uv_stream_t *client_stream) {
    uv_shutdown_t *shutdown_req = new uv_shutdown_t;

    uv_shutdown(shutdown_req, client_stream, on_shutdown);
  }
};

int main(int argc, char *argv[]) {
  uv_loop_t *loop = uv_default_loop();
  Server_Parameters sp(argc, argv, "h:p:d:");
  HTTP_Server server(loop, sp);

  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);

  return 0;
}
