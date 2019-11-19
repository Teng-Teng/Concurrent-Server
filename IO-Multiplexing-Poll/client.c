#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "wrap.h"

#define SERVER_PORT 7000
#define MAXLINE 1024

int main(int argc, char *argv[]) {
    int sockfd, n;
    char buf[MAXLINE];
    struct sockaddr_in server_addr;

    // memset(&server_addr, 0, sizeof(server_addr));
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(SERVER_PORT);         //  specify the port number

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    Connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("----------Connection success----------\n");
    
    while (fgets(buf, MAXLINE, stdin) != NULL) {
        Write(sockfd, buf, strlen(buf));    
        n = Read(sockfd, buf, MAXLINE);
        
        if (n == 0) {
            printf("The server has been closed.\n");
            break;
        }
        else
            Write(STDOUT_FILENO, buf, n);
    }
    Close(sockfd);
    
    return 0;
}




