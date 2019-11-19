#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#define SERVER_PORT 8080
#define MAX_EVENTS 1024
#define BUFLEN 4096

void recvdata(int fd, int events, void *arg);
void senddata(int fd, int events, void *arg);

//  description file descriptor information
struce myevent_s {
    inf fd;
    int events;
    void *arg;
    void (*call_back)(int fd, int events, void *arg);
    int status;                                         //   1-> listening on the red-black tree, 0-> not listening on the red-black tree
    char buf[BUFLEN];
    int len;
    long last_active;                                   //   when add to the red-black tree g_efd, record the time
};

int g_efd;                                              //  global variable, save the file descriptor returned by epoll_create
struct myevent_s g_events[MAX_EVENTS + 1];              //  [1, listenfd]

int main(int argc, char *argv[]) {
    unsigned short port = SERVER_PORT;
    if (argc == 2)
        port = atoi(argv[1]);                           //  use the port specified by user, if not specified, use the default port
        
    g_efd = epoll_create(MAX_EVENTS + 1);               //  create the red-black tree
    if (g_efd <= 0)
        printf("print efd in %s error %s\n", __func__, strerror(errno));
        
    initlistensocket(g_efd, port);                      //  initialize listening socket
    
    struct epoll_event events[MAX_EVENTS + 1];          //  save the file descriptors that meet the listening event
    print("server running on port[%d]\n", port);
    
    int checkpos = 0, i;
    while (1) {
        /*  timeout verification, test 10 connections each time，don't need to test for listenfd,
        if the client does not communicate with the server for 60 seconds, then close this client connection  */
        long now = time(NULL);                          //  current time
        for (i = 0; i < 100; i++, checkpos++) {
            if (checkpos == MAX_EVENTS)
                checkpos = 0;
            if (g_events[checkpos].status != 1)         //  not on the red-black tree
                continue;
                
            long duration = now - g_events[checkpos].last_active;           //  client inactive time
            if (duration >= 60) {
                close(g_events[checkpos].fd);           //  close the connection with the client
                printf("fd %d timeout\n", g_events[checkpos].fd);
                eventdel(g_efd, &g_events[checkpos]);   //  remove from the red-black tree g_efd
            }
        }
        
        /*  listening for the red-black tree, returns the number of file descriptors ready for the requested I/O,
        or zero if no file descriptor became ready during the requested timeout milliseconds*/
        int nfd = epoll_wait(g_efd, events, MAX_EVENTS + 1, 1000);      
        if (nfd < 0 ) {
            printf("epoll_wait error, exit\n");
            break;
        }
            
        for (i = 0; i < nfd; i++) {
            struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;
            
            if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))         //  read event
                ev->call_back(ev->fd, events[i].events, ev->arg);
                
            if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))       //  write event
                ev->call_back(ev->fd, events[i].events, ev->arg);
        }
    }
        
    return 0;
}











