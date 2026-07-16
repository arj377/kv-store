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
#define MAXDATASIZE 1024

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

int main(int argc, char *argv[]) {
    int status, sockfd, numBytes; //status for getaddrinfo, and sockets for communication
    struct addrinfo hints, *p;
    struct addrinfo *servinfo; //holds the linked list from getaddrinfo
    char buf[MAXDATASIZE]; //holds the string we receive
    char s[INET6_ADDRSTRLEN]; //holds the string to present in the terminal

    memset(&hints, 0, sizeof(hints)); //clear hints
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    //make sure a clall to the program and IP address is there
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
        //finds a valid socket
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("server: socket");
            continue;
        }
        
        //convert IP address to readable format and print
        if (inet_ntop(p->ai_family, get_in_addr(p->ai_addr), s, sizeof(s)) == NULL) {
            perror("inet_ntop");
            continue;
        }

        printf("client: attempting connection to %s\n", s);

        //make sure port is open
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
        
    }

    //make sure socket is found
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    
    //print readable IP address format
    inet_ntop(p->ai_family, get_in_addr(p->ai_addr), s, sizeof s);
    printf("client: connected to %s\n", s);

    freeaddrinfo(servinfo);

    //continue until "EXIT"
    while (true) {
        std::string input;
        std::getline(std::cin, input); //get user input

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
        
        //convert chars into a C++ string
        std::string response(buf, numBytes);
        std::cout << response << '\n';

        if (input == "EXIT") {
            break;
        }
    }
    
    //close the connection
    close(sockfd);
    return 0;
}