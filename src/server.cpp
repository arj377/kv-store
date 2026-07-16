#include <iostream>      
#include <sstream>      
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string>
#include "kv_store.h"

#define PORT "3490"
#define BACKLOG 10


//convert based on if its IPv6 or IPv4
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//keep sending until all the data is sent
bool sendAll(int socket, const std::string& message) {
    int total = message.size();
    int counter = 0;

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

int main() {
    int status, sockfd, new_fd; //status for getaddrinfo, and sockets for communication
    struct addrinfo hints, *p;
    struct addrinfo *servinfo; //holds the linked list from getaddrinfo
    int yes = 1; //for reuseaddr
    struct sockaddr_storage their_addr; //info about client's IP
    socklen_t addr_size;
    char s[INET6_ADDRSTRLEN]; //holds the string to present in the terminal

    memset(&hints, 0, sizeof(hints)); //clear hints first
    hints.ai_family = AF_UNSPEC; //IPv6 or IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; //this computer

    status = getaddrinfo(NULL, PORT, &hints, &servinfo); //load linked list into servinfo
    
    if (status != 0) {
        fprintf(stderr, "gai-error: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        //finds a valid socket
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("server: socket");
            continue;
        }

        //makes sure the socket can be reused
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            perror("setsockopt");
            close(sockfd);
            continue;
        }

        //make sure port is open
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        
        break;
    }

    freeaddrinfo(servinfo); //not needed anymore

    //make sure a valid socket was found
    if (p == NULL) {
        fprintf(stderr, "Server failed to bind\n");
        exit(1);
    }

    //wait for a client to connect
    if (listen(sockfd, BACKLOG) != 0) {
        perror("listen");
        exit(1);
    }

    std::cout << "KV Store: waiting for connections...\n";

    while (true) {
        addr_size = sizeof(their_addr);
        //accept any new connections
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        
        //convert client IP into readable form and print it
        if (inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof(s)) == NULL) {
            perror("inet_ntop");
            continue;
        }
        printf("server: got connection from %s\n", s);
        
        char buf[1024];

        //continue while client is connected or "EXIT"
        while(true) {
            int bytesRead = recv(new_fd, buf, sizeof(buf), 0);
            if (bytesRead == -1) {
                perror("recv");
                break;
            }
            else if (bytesRead == 0) {
                std::cout << "Client disconnected.\n";
                break;   // Go back to accept() and wait for another client
            }
            //convert chars to a string to operate with C++ functions
            std::string input (buf, bytesRead);
            if (input == "EXIT") {
                sendAll(new_fd, "Goodbye");
                std::cout << "Closing connection.\n";
                break;
            }
            std::string response = execute(input); //based on command, GET, SET, or DEL
            if (!sendAll(new_fd, response)) {
                perror("send");
                break;
            }  
        }
        //close this connection and set up a new one
        close(new_fd);

    }
    return 0;   
}