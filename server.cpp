#include <iostream>
#include <fstream>
#include <stdexcept>
// system headers
#include <unistd.h>
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

int main(int argc, char *argv[]) {
  Server_Parameters sp(argc, argv, "h:p:d:");
  
  return 0;
}