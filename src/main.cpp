#include <cstdlib>
#include <iostream>

#include "client.h"

inline void print_help() {
  std::cout << "Usage: ./swp2025_group7 ( server address | -s | --server ) ( "
               "port number | -p | --port )  [ --noprune ] [ -h | --help ]\n\n"
            << "Arguments:\n  -s, --server\t\t\tConnect to this hostname or "
               "IPv4/IPv6 address\n  -p, --port\t\t\tUse this port\n  "
               "--noprune\t\t\tDisable pruning\n\n"
            << "Basic options:\n  -h, --help\t\t\tShow this help text\n\n";
}

int main(int argc, char** argv) {
  std::cin.tie(NULL);

  std::string hostname, port;

  switch (argc) {
    case 2:
      if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
        print_help();
        return EXIT_SUCCESS;
      }
      [[fallthrough]];
    case 0: [[fallthrough]];
    case 1:
      std::cerr << "Missing server address and port\n";
      print_help();
      return EXIT_FAILURE;
    case 3: [[fallthrough]];
    case 4:
      for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "-h" || std::string(argv[i]) == "--help") {
          print_help();
          return EXIT_SUCCESS;
        }
      hostname = argv[1];
      port = argv[2];
      break;
    default:
      for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-s" || arg == "--server") && i + 1 < argc) {
          hostname = argv[++i];
          std::cout << "Passed server value '" << hostname << "'\n";
          std::cout << "Overwriting with '127.0.0.1'\n";
          hostname = "127.0.0.1";
        }
        else if ((arg == "-p" || arg == "--port") && i + 1 < argc)
          port = argv[++i];
        else if (arg == "-h" || arg == "--help") {
          print_help();
          return EXIT_SUCCESS;
        }
      }
  }
  if (hostname.empty()) {
    std::cerr << "Missing server address\n";
    return EXIT_FAILURE;
  }
  if (port.empty()) {
    std::cerr << "Missing port\n";
    return EXIT_FAILURE;
  }

  try {
    client client { std::move(hostname.data()), std::move(port.data()) };
    client.connect();
    client.listen();
  } catch (const std::exception& e) {
    std::cerr << "Connection error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

