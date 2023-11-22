# TCP Sockets Chat

## Overview

Contains files for a TCP server and client application that handles sending and receiving messages across multiple clients. The program keeps track of # of clients connected and last message sent from the client.

### Message structre

```cpp
struct tcpMessage
{
unsigned char nVersion;
unsigned char nType;
unsigned short nMsgLen;
char chMsg[1000];
};
```

#### Server Commands

- msg - prints out the last message received.
- clients - prints out the list of all clients connected to the server.
- exit closes all sockets and terminates the program.

#### Clients Commands

- v # - sets the version number (vVersion)
- t # message - sends server information on type of message being sent.
  - 77 sends to all clients except sender.
  - 201 sends to sender only.
- q - closes the stock and terminates the program.

## Usage

```css
make
./server [PORT]
./client [IP-ADDRESS] [PORT]
```
