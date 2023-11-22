/*
Author: your name
Class: ECE4122 or ECE6122 (A)
Last Date Modified: 11/21/2023
Description:
This file acts as a TCP client, it continously asks the end-user to input a command:
    1. v [nVersiion] changes the version number (ignores every message until v = 102)
    2. t [nType]
        a) if nType is 77 it sends to all clients except the sender.
        b) if nType is 201 it sends the reverse message to the sender.
        c) exit just closes connection
    3. q command closes connection to server for client.
*/
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

// Structure for TCP messages
struct tcpMessage
{
    unsigned char nVersion; // Version of the protocol (accepts 102 ignores others)
    unsigned char nType;    // Type of the message (77 normal or 201 reverse)
    unsigned short nMsgLen; // Length of the message
    char chMsg[1000];       // Message content
};

// Function to continuously receive messages from server
void receiveMessages(int serverSocket)
{
    tcpMessage msg;
    while (true)
    {
        std::memset(&msg, 0, sizeof(msg));
        ssize_t bytesReceived = recv(serverSocket, &msg, sizeof(msg), 0);
        if (bytesReceived <= 0)
        {
            std::cerr << "Connection closed." << std::endl;
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

    std::string serverIp = argv[1];
    int port = std::stoi(argv[2]);
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    // Check if input is 'localhost' and convert it to '127.0.0.1'
    if (serverIp == "localhost")
    {
        serverIp = "127.0.0.1";
    }

    if (serverSocket < 0)
    {
        std::cerr << "Cannot open socket" << std::endl;
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return 1;
    }

    if (connect(serverSocket, (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) < 0)
    {
        std::cerr << "Connection Failed" << std::endl;
        return 1;
    }

    std::cout << "Connected to the server at " << serverIp << ":" << port
              << std::endl;

    std::thread receiveThread(receiveMessages, serverSocket);

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
            // Send a disconnect message to the server
            msg.nType = 255;
            msg.nMsgLen = 0;
            send(serverSocket, &msg, sizeof(msg), 0);
            break;
        }
        else if (input[0] == 'v')
        {
            msg.nVersion = static_cast<unsigned char>(std::stoi(input.substr(2)));
        }
        else if (input[0] == 't')
        {
            size_t spacePos = input.find(' ', 2);
            msg.nType =
                static_cast<unsigned char>(std::stoi(input.substr(2, spacePos - 2)));
            std::string messageContent = input.substr(spacePos + 1);
            msg.nMsgLen = static_cast<unsigned short>(messageContent.size());
            std::strncpy(msg.chMsg, messageContent.c_str(), sizeof(msg.chMsg));
            msg.chMsg[sizeof(msg.chMsg) - 1] = '\0';

            send(serverSocket, &msg, sizeof(msg), 0);
        }
    }

    close(serverSocket);
    if (receiveThread.joinable())
        receiveThread.join();
    return 0;
}