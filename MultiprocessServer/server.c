#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "wrap.h"

#define SERVER_PORT 8888

void catch_child(int signum) {
    while (waitpid(0, NULL, WNOHANG) > 0);
    return;
}

int main(int argc, char* argv[]) {
    int lfd, cfd;
    int ret, i;
    char buf[BUFSIZ];
    char str[INET_ADDRSTRLEN];     //  #define INET_ADDRSTRLEN 16   use "[+d" to check
    pid_t pid;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    
    // memset(&server_addr, 0, sizeof(server_addr));
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);           //  specify the port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);     //  specify any local IP
    
    lfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Bind(lfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    Listen(lfd, 128);
    client_addr_len = sizeof(client_addr);
    
    while (1) {
        cfd = Accept(lfd, (struct sockaddr *)&client_addr, &client_addr_len);
	printf("receive from %s at port %d\n", 
                inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)),
                ntohs(client_addr.sin_port));    //  print client information(IP/PORT)
        
        pid = fork();
        if (pid < 0)
            perr_exit("fork error");
        else if (pid == 0) {
            close(lfd);
            break;
        } 
        else {
            struct sigaction act;
            act.sa_handler = catch_child;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;
            
            ret = sigaction(SIGCHLD, &act, NULL);
            if (ret != 0)
                perr_exit("sigaction error");
            
            close(cfd);
            continue;            
        }
    }
    
    if (pid == 0) {
        for (;;) {
            ret = Read(cfd, buf, sizeof(buf));    // read from the client
            if (ret == 0) {
		printf("the client %d closed...\n", cfd);
                close(cfd);
                exit(1);
            }
            
            for (i = 0; i < ret; i++) 
                buf[i] = toupper(buf[i]);     //  lowercase to uppercase
                
            write(cfd, buf, ret);	      //  write back to the client
            write(STDOUT_FILENO, buf, ret);   //  write to the screen
        }
    }
    
    return 0;
}




