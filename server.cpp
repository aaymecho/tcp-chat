#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

struct tcpMessage {
  unsigned char nVersion;
  unsigned char nType;
  unsigned short nMsgLen;
  char chMsg[1000];
};

std::map<int, sockaddr_in> clients; // Maps clientss and their addresses
std::mutex clients_mutex;

// Functions for future implementation
void handle_client(int client_socket);
void accept_connections(int server_socket);
void command_loop();

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return 1;
  }

  int port = std::stoi(argv[1]);
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    std::cerr << "Cannot open socket" << std::endl;
    return 1;
  }

  sockaddr_in server_address{};
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port);

  if (bind(server_socket, (sockaddr *)&server_address, sizeof(server_address)) <
      0) {
    std::cerr << "Cannot bind socket to port " << port << std::endl;
    return 1;
  }

  if (listen(server_socket, 5) < 0) {
    std::cerr << "Cannot listen on port" << std::endl;
    return 1;
  }

  std::cout << "Server is listening on port " << port << std::endl;
  return 0;
}
