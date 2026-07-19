#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include "kv_store.h"
#include <mutex>
#include <thread>

#define PORT "3490"
#define BACKLOG 10

std::mutex gLock;
std::mutex logMutex;

// Print so that multiple threads don't print at the same time
void log(const std::string &message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << message << '\n';
}

// Keep sending until all the data is sent
bool sendAll(int socket, const std::string &message)
{
    size_t total = message.size();
    size_t counter = 0;

    while (counter < total)
    {
        int charSent = send(socket, message.data() + counter, total - counter, 0);
        if (charSent <= 0)
        {
            return false;
        }
        else
        {
            counter += charSent;
        }
    }
    return true;
}

void handle_client(int socket)
{
    char buf[1024]; // Stores the user input
    // Run until the user exits
    while (true)
    {
        int bytesRead = recv(socket, buf, sizeof(buf), 0);
        if (bytesRead == -1)
        {
            perror("recv");
            break;
        }
        else if (bytesRead == 0)
        {
            log("Client disconnected.");
            break; // Go back to accept() and wait for another client
        }
        // Convert chars to a string to operate with C++ functions
        std::string input(buf, bytesRead);
        if (input == "EXIT")
        {
            if (!sendAll(socket, "Goodbye!"))
            {
                perror("send");
                break;
            }
            log("Closing connection.");
            break;
        }
        std::string response;
        { // Limit the lock's scope so the mutex is released before sendAll().
            std::lock_guard<std::mutex> clientLock(gLock);
            response = execute(input); // based on command, GET, SET, or DEL
        }
        if (!sendAll(socket, response))
        {
            perror("send");
            break;
        }
    }
    close(socket);
}

// Convert based on if its IPv6 or IPv4
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main()
{
    int status, sockfd, new_fd; // Status for getaddrinfo, and sockets for communication
    struct addrinfo hints, *p;
    struct addrinfo *servinfo;          // Holds the linked list from getaddrinfo
    int yes = 1;                        // For reuseaddr
    struct sockaddr_storage their_addr; // Info about client's IP
    socklen_t addr_size;
    char s[INET6_ADDRSTRLEN]; // Holds the string to present in the terminal

    memset(&hints, 0, sizeof(hints)); // Clear hints first
    hints.ai_family = AF_UNSPEC;      // IPv6 or IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // This computer

    status = getaddrinfo(NULL, PORT, &hints, &servinfo); // Load the linked list into servinfo

    if (status != 0)
    {
        fprintf(stderr, "gai-error: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // Finds a valid socket
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            perror("server: socket");
            continue;
        }

        // Makes sure the socket can be reused
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        {
            perror("setsockopt");
            close(sockfd);
            continue;
        }

        // Make sure port is open
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // Not needed anymore

    // Make sure a valid socket was found
    if (p == NULL)
    {
        fprintf(stderr, "Server failed to bind\n");
        exit(1);
    }

    // Wait for a client to connect
    if (listen(sockfd, BACKLOG) != 0)
    {
        perror("listen");
        exit(1);
    }

    std::cout << "KV Store: waiting for connections...\n";

    while (true)
    {
        addr_size = sizeof(their_addr);
        // Accept any new connections
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        // Convert client IP into readable form and print it
        if (inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s)) == NULL)
        {
            perror("inet_ntop");
            close(new_fd);
            continue;
        }
        log("server: got connection from " + std::string(s));

        std::thread client(&handle_client, new_fd); // Create separate client thread
        client.detach(); // Let that client thread run independently
    }
    return 0;
}