#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "wrap.h"

#define SERVER_PORT 7777
#define MAXLINE 80
#define OPEN_MAX 1024

int main(int argc, char *argv[]) {
    int i, j, maxi, listenfd, connfd, sockfd;
    int nready;     //  receive the return value from poll(), record the number of file descriptor that satisfy the listen event
    ssize_t n;
    
    char buf[MAXLINE], str[INET_ADDRSTRLEN];
    socklen_t client_addr_len;
    struct pollfd client[OPEN_MAX];
    struct sockaddr_in server_addr, client_addr;
    
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // memset(&server_addr, 0, sizeof(server_addr));
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);         //  specify the port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   //  specify any local IP

    Bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    Listen(listenfd, 128);
    
    client[0].fd = listenfd;    
    client[0].events = POLLIN;   //  listenfd listen for read event
    
    for (i = 0; i < OPEN_MAX; i++) 
        client[i].fd = -1;       // initialize the client array to -1
        
    maxi = 0;   

    for (; ;) {
        nready = poll(client, maxi + 1, -1);   //  blocks the server until a client connection request arrives
        if (nready == -1)
            perr_exit("poll error");
            
        if (client[0].revents & POLLIN) {      //  listenfd read event ready
            client_addr_len = sizeof(client_addr);
            connfd = Accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);    //  blocking I/O, receive client requests without blocking
	        printf("receive from %s at port %d\n", 
                   inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)),
                   ntohs(client_addr.sin_port));    //  print client information(IP/PORT)
                   
            for (i = 1; i < OPEN_MAX; i++) {
                if (client[i].fd < 0) {
                    client[i].fd = connfd;
                    break;
                }
            }
            
            if (i == OPEN_MAX)                     //   reached the maximum number of clients
                perr_exit("too many clients");   
                
            client[i].events = POLLIN;     //  connfd listen for read event
            if (i > maxi)
                maxi = i;
            if (--nready <= 0)
                continue;
        }
        
        for (i = 1; i <= maxi; i++) {
            if ((sockfd = client[i].fd) < 0)
                continue;
                
            if (client[i].revents & POLLIN) {
                if ((n = Read(sockfd, buf, MAXLINE)) < 0) {
                    if (errno == ECONNRESET) {    //  receive rst flag
                        printf("client %d aborted connection\n", client[i].fd);
                        Close(sockfd);
                        client[i].fd = -1;
                    }
                    else
                        perr_exit("read error");
                } else if (n == 0) {     //  client closed the connection
                    printf("client %d closed connection\n", client[i].fd);
                    Close(sockfd);
                    client[i].fd = -1;
                } else {
                    for (j = 0; j < n; j++)
                        buf[j] = toupper(buf[j]);
                    Write(sockfd, buf, n);    
                }
                if (--nready <= 0)
                    break;
            }
        }
    }
    
    return 0;
}








