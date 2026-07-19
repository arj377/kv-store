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

#define PORT "3490"
#define MAXDATASIZE 1024

// Convert based on if its IPv6 or IPv4
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Keep sending until all the data is sent
bool sendAll(int socket, const std::string& message) {
    size_t total = message.size();
    size_t counter = 0;

    while (counter < total) {
        int charSent = send(socket, message.data() + counter, total - counter, 0);
        if (charSent <= 0) {
            return false;
        }
        else {
            counter += charSent;
        }  
    }
    return true;
}

int main(int argc, char *argv[]) {
    int status, sockfd, numBytes; // Status for getaddrinfo, and sockets for communication
    struct addrinfo hints, *p;
    struct addrinfo *servinfo; // Holds the linked list from getaddrinfo
    char buf[MAXDATASIZE]; // Holds the string we receive
    char s[INET6_ADDRSTRLEN]; // Holds the string to present in the terminal

    memset(&hints, 0, sizeof(hints)); // Clear hints
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    // Make sure a clall to the program and IP address is there
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " hostname\n";
        return 1;
    }

    status = getaddrinfo(argv[1], PORT, &hints, &servinfo);
    if (status != 0) {
        fprintf(stderr, "gai-error: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        // Finds a valid socket
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("server: socket");
            continue;
        }
        
        // Convert IP address to readable format and print
        if (inet_ntop(p->ai_family, get_in_addr(p->ai_addr), s, sizeof(s)) == NULL) {
            perror("inet_ntop");
            continue;
        }

        printf("client: attempting connection to %s\n", s);

        // Make sure port is open
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
        
    }

    // Make sure socket is found
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    
    // Print readable IP address format
    inet_ntop(p->ai_family, get_in_addr(p->ai_addr), s, sizeof s);
    printf("client: connected to %s\n", s);

    freeaddrinfo(servinfo);

    // Continue until "EXIT"
    while (true) {
        std::string input;
        std::getline(std::cin, input); // Get user input

        if (!sendAll(sockfd, input)) {
            perror("send");
            break;
        }
        
        numBytes = recv(sockfd, buf, sizeof(buf), 0);
        if (numBytes == -1) {
            perror("recv");
            exit(1);
        }

        else if (numBytes == 0) {
            std::cout << "Server disconnected.\n";
            break;   // Go back to accept() and wait for another client
        }
        
        // Convert chars into a C++ string
        std::string response(buf, numBytes);
        std::cout << response << '\n';

        if (input == "EXIT") {
            break;
        }
    }
    
    // Close the connection
    close(sockfd);
    return 0;
}