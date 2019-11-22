#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
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

#define SERVER_PORT 8888

/*  callback to invoke when there is data to be read  */
void read_cb(struct bufferevent *bev, void *arg) {
    char buf[1024] = {0};
    
    bufferevent_read(bev, buf, sizeof(buf));                                //  read data from a bufferevent buffer
    printf("data sent by the client: %s\n", buf);
    
    char *p = "The server has successfully received the data you sent!";
    bufferevent_write(bev, p, strlen(p) + 1);                               //  write data to a bufferevent buffer(write back to the client)
    sleep(1);
}

/*  callback to invoke when the file descriptor is ready for writing  */
void write_cb(struct bufferevent *bev, void *arg) {
    printf("The server successfully sent data to the client, called the write buffer callback function...\n");
}

/*  callback to invoke when there is an event on the file descriptor  */
void event_cb(struct bufferevent *bev, short events, void *arg) {
    if (events & BEV_EVENT_EOF)
        printf("connection closed\n");
    else if (events & BEV_EVENT_ERROR)
        printf("some other error\n");
        
    bufferevent_free(bev);                                                  //  deallocate the storage associated with a bufferevent structure
    printf("deallocate the storage associated with a bufferevent structure...\n");
}

/*  a callback that we invoke when a listener encounters a non-retriable error  */
void cb_listener(struct evconnlistener *listener, evutil_socket_t sockfd, struct sockaddr *addr, int socklen, void *ptr) {
    printf("connect with a new client\n");
    struct event_base* base = (struct event_base*)ptr;
    
    struct bufferevent *bev;
    bev = bufferevent_socket_new(base, sockfd, BEV_OPT_CLOSE_ON_FREE);          //  create a new socket bufferevent over an existing socket
                                                                            
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);              //  changes the callbacks for a bufferevent
    bufferevent_enable(bev, EV_READ);                                       //  enable read bufferevent
    return;
}   

int main(int argc, const char* argv[]) {
    struct sockaddr_in server_addr;
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);          //  specify the port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    //  specify any local IP
    
    struct event_base* base;
    base = event_base_new();                            //  create an event_base
    
    struct evconnlistener* listener;                    //  connection listener
    listener = evconnlistener_new_bind(base, cb_listener, base, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 36, (struct sockaddr *)&server_addr, sizeof(server_addr));          //  connection listeners: accepting TCP connections
    
    event_base_dispatch(base);                          //  event dispatching loop
    evconnlistener_free(listener);                      //  disable and deallocate an evconnlistener
    event_base_free(base);                              //  deallocate all memory associated with an event_base, and free the base

    return 0;
}






