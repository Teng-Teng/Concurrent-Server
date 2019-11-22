#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#ifndef _WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include "wrap.h"

#define SERVER_PORT 8888

/*  callback to invoke when there is data to be read  */
void read_cb(struct bufferevent *bev, void *arg) {
    char buf[1024] = {0};
    
    bufferevent_read(bev, buf, sizeof(buf));                                //  read data from a bufferevent buffer
    printf("data sent by the server: %s\n", buf);
    
    bufferevent_write(bev, buf, strlen(buf) + 1);                           //  write data to a bufferevent buffer(write to the server)
    sleep(1);
}

/*  callback to invoke when the file descriptor is ready for writing  */
void write_cb(struct bufferevent *bev, void *arg) {
    printf("The client successfully sent data to the server, called the write buffer callback function...\n");
}

/*  callback to invoke when there is an event on the file descriptor  */
void event_cb(struct bufferevent *bev, short events, void *arg) {
    if (events & BEV_EVENT_EOF)
        printf("connection closed\n");
    else if (events & BEV_EVENT_ERROR)
        printf("some other error\n");
    else if (events & BEV_EVENT_CONNECTED) {
        printf("Connected to the server\n");
        return;  
    }
        
    bufferevent_free(bev);                                                  //  deallocate the storage associated with a bufferevent structure
    printf("deallocate the storage associated with a bufferevent structure...\n");
}

/*  client interact with the user, read data from the terminal and send it to the server  */
void read_terminal(evutil_socket_t fd, short what, void *arg) {
    char buf[1024] = {0};
    int len = Read(fd, buf, sizeof(buf));
    
    struct bufferevent* bev = (struct bufferevent*)arg;
    bufferevent_write(bev, buf, len + 1);
}

int main(int argc, const char* argv[]) {
    struct sockaddr_in server_addr;
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(SERVER_PORT);   
    
    struct event_base* base = NULL;
    base = event_base_new();                                                //  create an event_base
    
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    struct bufferevent* bev = NULL;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);          //  create a new socket bufferevent over an existing socket

    bufferevent_socket_connect(bev, (struct sockaddr *)&server_addr, sizeof(server_addr));      //  launch a connect() attempt with a socket-based bufferevent
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);              //  changes the callbacks for a bufferevent
    //bufferevent_enable(bev, EV_READ);                                     //  enable read bufferevent
    struct event* ev = event_new(base, STDIN_FILENO, EV_READ | EV_PERSIST, read_terminal, bev);     //  allocate and asssign a new event structure, ready to be added
    event_add(ev, NULL);                                                    //  add an event to the set of pending events

    event_base_dispatch(base);                                              //  event dispatching loop
    event_free(ev);                                                         //  deallocate a struct event * returned by event_new()
    event_base_free(base);                                                  //  deallocate all memory associated with an event_base, and free the base

    return 0;
}






