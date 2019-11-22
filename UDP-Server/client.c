#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "wrap.h"

#define SERVER_PORT 8000

#define SERVER_IP "127.0.0.1"

int main(int argc, char *argv[]) {
    int sockfd, ret;
    char buf[BUFSIZ];
    struct sockaddr_in server_addr;

    // memset(&server_addr, 0, sizeof(server_addr));
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(SERVER_PORT);         //  specify the port number

    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    printf("----------Connection success----------\n");

    while (1) {
        fgets(buf, sizeof(buf), stdin); 

        ret = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret == -1)
            perror("sendto error");
  
        ret = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, 0);
        if (ret == -1)
            perror("recvfrom error");
            
        Write(STDOUT_FILENO, buf, ret);   //  write to the screen
    }

    Close(sockfd);
    return 0;
}








