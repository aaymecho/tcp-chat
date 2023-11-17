#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

struct tcpMessage {
  unsigned char nVersion;
  unsigned char nType;
  unsigned short nMsgLen;
  char chMsg[1000];
};

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <IP Address> <Port>" << std::endl;
    return 1;
  }

  std::string server_ip = argv[1];
  int port = std::stoi(argv[2]);
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    std::cerr << "Cannot open socket" << std::endl;
    return 1;
  }

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
    std::cerr << "Invalid address/ Address not supported" << std::endl;
    return 1;
  }

  if (connect(server_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    std::cerr << "Connection Failed" << std::endl;
    return 1;
  }

  std::cout << "Connected to the server at " << server_ip << ":" << port
            << std::endl;
}
