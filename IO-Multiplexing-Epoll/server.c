#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <ctype.h>
#include "wrap.h"

#define SERVER_PORT 7777
#define MAXLINE 8192
#define OPEN_MAX 5000

int main(int argc, char *argv[]) {
    int i, n, listenfd, connfd, sockfd;
    ssize_t nready, efd, res;
    char buf[MAXLINE], str[INET_ADDRSTRLEN];     //  #define INET_ADDRSTRLEN 16   use "[+d" to check
    socklen_t client_addr_len;

    struct sockaddr_in server_addr, client_addr;
    struct epoll_event tep, ep[OPEN_MAX];
    
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));   //  reuse port
    
    // memset(&server_addr, 0, sizeof(server_addr));
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);         //  specify the port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   //  specify any local IP
    
    Bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    Listen(listenfd, 128);
    
    efd = epoll_create(OPEN_MAX);
    if (efd == -1)
        perr_exit("epoll_create error");
        
    tep.events = EPOLLIN;
    tep.data.fd = listenfd;    //  listenfd listen for read event
    
    res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);
    if (res == -1)
        perr_exit("epoll_ctl error");
        
    for ( ; ; ) {
        nready = epoll_wait(efd, ep, OPEN_MAX, -1);
        if (nready == -1)
            perr_exit("epoll_wait error");
            
        for (i = 0; i < nready; i++) {
            if(!(ep[i].events & EPOLLIN))
                continue;
                
            if (ep[i].data.fd == listenfd) {
                client_addr_len = sizeof(client_addr);
                connfd = Accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
        	    printf("receive from %s at port %d\n", 
                        inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)),
                        ntohs(client_addr.sin_port));    //  print client information(IP/PORT)
                printf("client %d is connected\n", connfd - 4);
                
                tep.events = EPOLLIN;
                tep.data.fd = connfd;    //  connfd listen for read event
                
                res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
                if (res == -1)
                    perr_exit("epoll_ctl error");
                
            } else {
                sockfd = ep[i].data.fd;
                n = Read(sockfd, buf, MAXLINE);
                
                if (n == 0) {    //  client closed connection
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);    //  Remove (deregister) the target file descriptor sockfd from the interest list
                    if (res == -1)
                        perr_exit("epoll_ctl error");
                    Close(sockfd);
                    printf("client %d closed connection\n", sockfd - 4);
                        
                } else if (n < 0) {
                    perror("read error");
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);    //  Remove (deregister) the target file descriptor sockfd from the interest list
                    if (res == -1)
                        perr_exit("epoll_ctl error");
                    Close(sockfd);
                    
                } else {
                    for (i = 0; i < n; i++) 
                        buf[i] = toupper(buf[i]);
                        
                    Write(STDOUT_FILENO, buf, n);
                    Write(sockfd, buf, n);
                }
            }
        }
    }
    Close(listenfd);
    Close(efd);
    
    return 0;
}









