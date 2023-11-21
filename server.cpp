#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

struct tcpMessage
{
  unsigned char nVersion;
  unsigned char nType;
  unsigned short nMsgLen;
  char chMsg[1000];
};

std::map<int, sockaddr_in> clients;
std::mutex clients_mutex;
tcpMessage lastReceivedMessage;
bool messageAvailable = false;

void reverse_message(tcpMessage &msg)
{
  std::reverse(msg.chMsg, msg.chMsg + msg.nMsgLen);
}

void send_message_to_all_clients(int sender_socket, const tcpMessage &msg)
{
  std::lock_guard<std::mutex> lock(clients_mutex);
  for (const auto &client : clients)
  {
    if (client.first != sender_socket)
    {
      send(client.first, &msg, sizeof(msg), 0);
    }
  }
}

void handle_client(int client_socket)
{
  sockaddr_in client_address;
  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    client_address = clients[client_socket];
  }

  while (true)
  {
    tcpMessage msg;
    ssize_t bytes_received = recv(client_socket, &msg, sizeof(msg), 0);
    if (bytes_received <= 0)
      break;

    // Update the last message received
    {
      std::lock_guard<std::mutex> lock(clients_mutex);
      lastReceivedMessage = msg;
      messageAvailable = true;
    }

    if (msg.nVersion == 102)
    {
      if (msg.nType == 77)
      {
        send_message_to_all_clients(client_socket, msg);
      }
      else if (msg.nType == 201)
      {
        reverse_message(msg);
        send(client_socket, &msg, sizeof(msg), 0);
      }
    }
  }

  close(client_socket);
  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(client_socket);
    std::cout << "Client disconnected: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << std::endl;
  }
}

void command_loop()
{
  std::string command;
  while (true)
  {
    std::cout << "Please enter command: ";
    std::cin >> command;
    if (command == "msg")
    {
      std::lock_guard<std::mutex> lock(clients_mutex);
      if (messageAvailable)
      {
        std::cout << "Last Message: " << lastReceivedMessage.chMsg << std::endl;
      }
      else
      {
        std::cout << "No messages received yet." << std::endl;
      }
    }
    else if (command == "clients")
    {
      std::lock_guard<std::mutex> lock(clients_mutex);
      std::cout << "Number of Clients: " << clients.size() << std::endl;
      for (const auto &client : clients)
      {
        std::cout << "IP Address: " << inet_ntoa(client.second.sin_addr) << " | Port: " << ntohs(client.second.sin_port) << std::endl;
      }
    }
    else if (command == "exit")
    {
      break;
    }
    else
    {
      std::cout << "Unknown command." << std::endl;
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return 1;
  }

  int port = std::stoi(argv[1]);
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0)
  {
    std::cerr << "Cannot open socket" << std::endl;
    return 1;
  }

  sockaddr_in server_address{};
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port);

  if (bind(server_socket, (sockaddr *)&server_address, sizeof(server_address)) < 0)
  {
    std::cerr << "Cannot bind socket to port " << port << std::endl;
    return 1;
  }

  if (listen(server_socket, 5) < 0)
  {
    std::cerr << "Cannot listen on port" << std::endl;
    return 1;
  }

  std::cout << "Server is listening on port " << port << std::endl;

  std::thread accept_thread(accept_connections, server_socket);
  command_loop();

  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto &client : clients)
    {
      close(client.first);
    }
    clients.clear();
  }

  close(server_socket);
  accept_thread.join();

  return 0;
}

void accept_connections(int server_socket)
{
  while (true)
  {
    sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    int client_socket = accept(server_socket, (sockaddr *)&client_address, &client_address_len);

    if (client_socket < 0)
    {
      std::cerr << "Failed to accept client connection." << std::endl;
      continue;
    }

    {
      std::lock_guard<std::mutex> lock(clients_mutex);
      clients[client_socket] = client_address;
    }

    std::thread client_thread(handle_client, client_socket);
    client_thread.detach();
  }
}
