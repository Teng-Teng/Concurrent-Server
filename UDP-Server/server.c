#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "wrap.h"

#define SERVER_PORT 8000

int main(void) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    int sockfd;
    char buf[BUFSIZ];
    char str[INET_ADDRSTRLEN];     //  #define INET_ADDRSTRLEN 16   use "[+d" to check
    int i, n;
    
    // memset(&server_addr, 0, sizeof(server_addr));
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);           //  specify the port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);     //  specify any local IP
    
    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    Bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Accepting connections...\n");
    
    while (1) {
        client_addr_len = sizeof(client_addr);
        n = recvfrom(sockfd, buf, BUFSIZ, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (n == -1)
            perror("recvfrom error");
            
        printf("receive from %s at port %d\n", 
                inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)),
                ntohs(client_addr.sin_port));    //  print client information(IP/PORT)
                
        for (i = 0; i < n; i++) 
            buf[i] = toupper(buf[i]);     //  lowercase to uppercase

        n = sendto(sockfd, buf, n, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
        if (n == -1)
            perror("sendto error");
    }
    
    close(sockfd);
    return 0;
}







