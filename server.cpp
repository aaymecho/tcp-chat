/*
Author: your name
Class: ECE4122 or ECE6122 (A)
Last Date Modified: 11/21/2023
Description:
This file acts as a TCP server which itcontinuously asks for user command while handling multiple
client connections, making it so that it recieves/sending messages:
    1. Sends all clients except the sender the message if nType is 77
    2. Sends sender reverse message if nType is 201.
    3. msg (prints last message saved from client)
    4. exit closes connection of server and clients
One thing to note is that the server will ignore any messages until the client has
set the nVersion (version protocol) to 102 and remains at 102. One additional feature, is that
it also lists the clients connection and +their information (used by typing clients in server command).
*/
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
#include <atomic>
#include <signal.h>

/*----------------------------------Function-Definitions-Start----------------------------------*/
// Structure for TCP messages
struct tcpMessage
{
    unsigned char nVersion; // Version of the protocol (accepts 102 ignores others)
    unsigned char nType;    // Type of the message (77 normal or 201 reverse)
    unsigned short nMsgLen; // Length of the message
    char chMsg[1000];       // Message content
};

// Global variables
std::map<int, sockaddr_in> clients;
std::mutex clientsMutex;
tcpMessage lastReceivedMessage;
bool messageAvailable = false;
std::atomic<bool> serverRunning(true); // Manage server state with safety (regular bool has restrictions)

// Function to reverse the message content
void reverseMessage(tcpMessage &msg)
{
    std::reverse(msg.chMsg, msg.chMsg + msg.nMsgLen);
}

// Function to send a message to all clients except the sender
void sendMessageToAllClients(int senderSocket, const tcpMessage &msg)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto &client : clients)
    {
        if (client.first != senderSocket)
        {
            send(client.first, &msg, sizeof(msg), 0);
        }
    }
}

// Function to handle individual client connections
void handleClient(int clientSocket)
{
    sockaddr_in clientAddress;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clientAddress = clients[clientSocket];
    }

    while (true)
    {
        tcpMessage msg;
        ssize_t bytesReceived = recv(clientSocket, &msg, sizeof(msg), 0);
        if (bytesReceived <= 0)
            break;

        // Detect disconnection message
        if (msg.nType == 255)
        {
            std::cout << "Client requested disconnection: " << inet_ntoa(clientAddress.sin_addr)
                      << ":" << ntohs(clientAddress.sin_port) << std::endl;
            break; // Break out of the loop to close the socket
        }

        if (msg.nVersion == 102)
        {
            if (msg.nType == 77)
            {
                sendMessageToAllClients(clientSocket, msg);

                // Update the last message received
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    lastReceivedMessage = msg;
                    messageAvailable = true;
                }
            }
            else if (msg.nType == 201)
            {
                reverseMessage(msg);
                send(clientSocket, &msg, sizeof(msg), 0);

                // Update the last message received
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    lastReceivedMessage = msg;
                    messageAvailable = true;
                }
            }
        }
        else
        {
            continue;
        }
    }

    close(clientSocket);
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(clientSocket);
        std::cout << "Client disconnected: " << inet_ntoa(clientAddress.sin_addr)
                  << ":" << ntohs(clientAddress.sin_port) << std::endl;
    }
}

// Function for the main command loop (continuously asks for end-user to input command)
void commandLoop()
{
    std::string command;
    while (true)
    {
        std::cout << "Please enter command: ";
        std::cin >> command;
        if (command == "msg")
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
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
            std::lock_guard<std::mutex> lock(clientsMutex);
            std::cout << "Number of Clients: " << clients.size() << std::endl;
            for (const auto &client : clients)
            {
                std::cout << "IP Address: " << inet_ntoa(client.second.sin_addr)
                          << " | Port: " << ntohs(client.second.sin_port) << std::endl;
            }
        }
        else if (command == "exit")
        {
            // Notify clients of server shutdown
            tcpMessage shutdownMsg;
            shutdownMsg.nVersion = 102;
            shutdownMsg.nType = 77; // Normal message type
            strcpy(shutdownMsg.chMsg, "Server is shutting down.");
            shutdownMsg.nMsgLen = strlen(shutdownMsg.chMsg);
            sendMessageToAllClients(-1, shutdownMsg); // -1 indicates sending to all clients

            serverRunning = false; // Set the server running flag to false
            break;
        }
        else
        {
            std::cout << "Unknown command." << std::endl;
        }
    }
}

// Function to accept incoming client connections
void acceptConnections(int serverSocket)
{
    while (serverRunning)
    {
        sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);
        int clientSocket =
            accept(serverSocket, (sockaddr *)&clientAddress, &clientAddressLen);

        if (clientSocket < 0)
        {
            if (!serverRunning)
            {
                break; // Break if the server is no longer running
            }
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients[clientSocket] = clientAddress;
        }

        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }
}

/*----------------------------------Function-Definitions-End----------------------------------*/

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        std::cerr << "Cannot open socket" << std::endl;
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        std::cerr << "Cannot bind socket to port " << port << std::endl;
        return 1;
    }

    if (listen(serverSocket, 5) < 0)
    {
        std::cerr << "Cannot listen on port" << std::endl;
        return 1;
    }

    std::cout << "Server is listening on port " << port << std::endl;
    std::thread acceptThread(acceptConnections, serverSocket);
    commandLoop();

    shutdown(serverSocket, SHUT_RDWR); // Stop accepting new connections

    // Scope created to automatically destroy lock_guard
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (const auto &client : clients)
        {
            close(client.first);
        }
        clients.clear();
    }

    close(serverSocket);
    acceptThread.join();
    return 0;
}