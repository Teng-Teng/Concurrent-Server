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

/*  Initialize member variables of struct myevent_s  */
void eventset(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg) {
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    memset(ev->buf, 0, sizeof(ev->buf));
    ev->len = 0;
    ev->last_active = time(NULL);
    
    return;
}

/*  Establish a connection with the client  */
void acceptconn(int lfd, int events, void *arg) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int cfd, i;
    
    if ((cfd = accept(lfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
        if (errno != EAGAIN && errno != EINTR) {
            //  error handling
        }
        printf("%s: accept, %s\n", __func__, strerror(errno));
        return;
    }
    
    do {
        for (i = 0; i < MAX_EVENTS; i++)
            if (g_events[i].status == 0)
                break;
                
        if (i == MAX_EVENTS) {
            printf("%s: max connection limit %d\n", __func__, MAX_EVENTS);
            break;
        }
        
        int flag = 0;
        if ((flag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0) {         //  set cfd to non-blocking
            printf("%s: function fcntl failed to set cfd to non-blocking, %s\n", __func__, strerror(errno));
            break;
        }
        
        eventset(&g_events[i], cfd, recvdata, &g_events[i]);        //  set a struct myevent_s for cfd, set callback function to recvdata
        eventadd(g_efd, EPOLLIN, &g_events[i]);                       //  add cfd to the red-black tree, listen for read event
        
    } while(0);
    
    printf("new connection [%s: %d][time: %ld], pos[%d]\n", 
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), g_events[i].last_active, i);
    
}



/*  Create socket, initialize listenfd */
void initListenSocket(int efd, short port) {
    struct sockaddr_in server_addr;
    
    inf lfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(lfd, F_SETFL, O_NONBLOCK);                    //  set socket to non-blocking
    
    // memset(&server_addr, 0, sizeof(server_addr));
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);         //  specify the port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   //  specify any local IP
    
    bind(lfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(lfd, 128);
    
    eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);
    eventadd(efd, EPOLLIN, &g_events[MAX_EVENTS]);
}

int main(int argc, char *argv[]) {
    unsigned short port = SERVER_PORT;
    if (argc == 2)
        port = atoi(argv[1]);                           //  use the port specified by user, if not specified, use the default port
        
    g_efd = epoll_create(MAX_EVENTS + 1);               //  create the red-black tree
    if (g_efd <= 0)
        printf("print efd in %s error %s\n", __func__, strerror(errno));
        
    initListenSocket(g_efd, port);                      //  initialize listening socket
    
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







