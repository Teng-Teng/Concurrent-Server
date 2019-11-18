#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include "wrap.h"

#define SERVER_PORT 8000
#define MAXLINE 8192

struct s_info {
    struct sockaddr_in client_addr;
    int connfd;
};

void *do_work(void *arg) {
    int n, i;
    struct s_info *ts = (struct s_info*)arg;
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN];     //  #define INET_ADDRSTRLEN 16   use "[+d" to check
    
    while (1) {
        n = Read(ts->connfd, buf, MAXLINE);   // read from the client
        if (n == 0) {
            printf("the client %d closed...\n", ts->connfd);
            break;
        }
        printf("receive from %s at port %d\n", 
                inet_ntop(AF_INET, &(*ts).client_addr.sin_addr, str, sizeof(str)),
                ntohs((*ts).client_addr.sin_port));    //  print client information(IP/PORT)
                
        for (i = 0; i < n; i++)
            buf[i] = toupper(buf[i]);   //  lowercase to uppercase
            
        Write(STDOUT_FILENO, buf, n);   //  write to the screen
        Write(ts->connfd, buf, n);      //  write back to the client 
    }
    Close(ts->connfd);
    
    return (void *)0;   // pthread_exit(0);
}

int main(void) {
    int i = 0;
    struct s_info ts[256];
    
    int listenfd, connfd;
    pthread_t tid;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    
    // memset(&server_addr, 0, sizeof(server_addr));
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);         //  specify the port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   //  specify any local IP
    
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    Listen(listenfd, 128);
    client_addr_len = sizeof(client_addr);
    printf("Accepting client connection...\n");
    
    while (1) {
        connfd = Accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
        ts[i].client_addr = client_addr;
        ts[i].connfd = connfd;
    
        pthread_create(&tid, NULL, do_work, (void*)&ts[i]);
        pthread_detach(tid);
        i++;
    }
    
    return 0;
}
