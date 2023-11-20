#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

struct tcpMessage
{
  unsigned char nVersion;
  unsigned char nType;
  unsigned short nMsgLen;
  char chMsg[1000];
};

void receive_messages(int server_socket)
{
  tcpMessage msg;
  while (true)
  {
    std::memset(&msg, 0, sizeof(msg));
    ssize_t bytes_received = recv(server_socket, &msg, sizeof(msg), 0);
    if (bytes_received <= 0)
    {
      std::cerr << "Connection closed or error occurred." << std::endl;
      break;
    }
    std::cout << "Received Msg Type: " << static_cast<int>(msg.nType)
              << "; Msg: " << msg.chMsg << std::endl;
  }
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <IP Address> <Port>" << std::endl;
    return 1;
  }

  std::string server_ip = argv[1];
  int port = std::stoi(argv[2]);
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0)
  {
    std::cerr << "Cannot open socket" << std::endl;
    return 1;
  }

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0)
  {
    std::cerr << "Invalid address/ Address not supported" << std::endl;
    return 1;
  }

  if (connect(server_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0)
  {
    std::cerr << "Connection Failed" << std::endl;
    return 1;
  }

  std::cout << "Connected to the server at " << server_ip << ":" << port
            << std::endl;

  std::thread receive_thread(receive_messages, server_socket);

  std::string input;
  tcpMessage msg;
  while (true)
  {
    std::cout << "Please enter command: ";
    std::getline(std::cin, input);

    if (input.empty())
      continue;

    if (input[0] == 'q')
    {
      break;
    }
    else if (input[0] == 'v')
    {
      msg.nVersion = static_cast<unsigned char>(std::stoi(input.substr(2)));
    }
    else if (input[0] == 't')
    {
      size_t space_pos = input.find(' ', 2);
      msg.nType = static_cast<unsigned char>(std::stoi(input.substr(2, space_pos - 2)));
      std::string message_content = input.substr(space_pos + 1);
      msg.nMsgLen = static_cast<unsigned short>(message_content.size());
      std::strncpy(msg.chMsg, message_content.c_str(), sizeof(msg.chMsg));
      msg.chMsg[sizeof(msg.chMsg) - 1] = '\0';

      send(server_socket, &msg, sizeof(msg), 0);
    }
  }

  close(server_socket);
  receive_thread.join();

  return 0;
}
