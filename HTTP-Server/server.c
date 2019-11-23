#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

#define MAXSIZE 2048

/*  create listen file descriptor, add it to the red-black tree  */
int init_listen_fd(int port, int epfd) {
    int lfd = socket(AF_INET, SOCK_STREAM, 0);                  //  create an endpoint for communication
    if (lfd == -1) {
        perror("socket error");
        exit(1);
    }
    
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));                   // memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);                         //  specify the port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);            //  specify any local IP

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));   //  reuse port

    int ret = bind(lfd, (struct sockaddr *)&server_addr, sizeof(server_addr));      //  bind a name to a socket
    if (ret == -1) {
        perror("bind error");
        exit(1);
    }
    
    ret = listen(lfd, 128);                                     //  listen for connections on a socket
    if (ret == -1) {
        perror("listen error");
        exit(1);
    }


    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;                                           //  lfd listen for read event
    
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if (ret == -1) {
        perror("epoll_ctl add listen file descriptor error");
        exit(1);
    }
    
    return lfd;
}

void do_accept(int lfd, int epfd) {
    
}

void do_read(int cfd, int epfd) {
    
}

/*  use epoll to wait for an I/O event  */
void epoll_run(int port) {
    int i = 0;
    struct epoll_event all_events[MAXSIZE];
    
    int efd = epoll_create(MAXSIZE);                            //  open an epoll file descriptor
    if (efd == -1) {
        perror("epoll_create error");
        exit(1);        
    }
    
    int lfd = init_listen_fd(port, epfd);                       //  create listen file descriptor, add it to the red-black tree
    
    while (1) {
        int ret = epoll_wait(efd, all_events, MAXSIZE, -1);     //  wait for an I/O event on an epoll file descriptor
        if (ret == -1) {
            perror("epoll_wait error");
            exit(1);
        }
            
        for (i = 0; i < ret; ++i) {
            struct epoll_event *pev = &all_events[i];           //  only handle read events, do not handle other events by default    

        }
    }
    
    
    
}

int main(int argc, char *argv[]) {
    if (argc < 3)
        printf("./server port path\n");                         //  get command line arguments, port and server directory
        
    int port = atoi(argv[1]);                                   //  get the port entered by the user
    
    int ret = chdir(argv[2]);                                   //  change working directory
    if (ret != 0) {
        perror("chdir error");
        exit(1);
    }
    
    epoll_run(port);                            //  use epoll to wait for an I/O event
    return 0;
}





