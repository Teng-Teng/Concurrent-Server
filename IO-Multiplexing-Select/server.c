#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "wrap.h"

#define SERVER_PORT 6666

int main(int argc, char *argv[]) {
    int listenfd, connfd;
    char buf[BUFSIZ];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    
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
    
    fd_set rset, allset;
    int ret, maxfd = 0, n, i, j;
    maxfd = listenfd;
    
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    while (1) {
        rset = allset;
        ret = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (ret < 0) 
            perr_exit("select error");
            
        if (FD_ISSET(listenfd, &rset)) {
            client_addr_len = sizeof(client_addr);
            connfd = Accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
            FD_SET(connfd, &allset);
            
            if (maxfd < connfd)
                maxfd = connfd;
                
            if (ret == 1)
                continue;
        }
        
        for (i = listenfd + 1; i <= maxfd; i++) {
            if (FD_ISSET(i, &rset)) {
                n = Read(i, buf, sizeof(buf));
                if (n == 0) {
                    Close(i);
                    FD_CLR(i, &allset);
                } 
                else if (n == -1)
                    perr_exit("read error");
                    
                for (j = 0; j < n; j++) 
                    buf[j] = toupper(buf[j]);
                    
                Write(i, buf, n);
                Write(STDOUT_FILENO, buf, n);
            }
        }
    }
    
    return 0;
}





