#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include "libevent_http.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("./event_http port path\n");                     //  get command line arguments, port and server directory
        return -1;        
    }
        
    if (chdir(argv[2]) < 0) {                                    //  change working directory
        printf("directory doesn't exist: %s\n", argv[2]);
        perror("chdir error: ");
        return -1;
    }
    
    struct event_base* base;
    struct evconnlistener *listener;                    //  connection listener
    struct event *signal_event;
    struct sockaddr_in server_addr;
    
    base = event_base_new();                            //  create an event_base
    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));          //  specify the port number
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    //  specify any local IP
    
    listener = evconnlistener_new_bind(base, listener_cb, (void *)base, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, (struct sockaddr *)&server_addr, sizeof(server_addr));          //  connection listeners: accepting TCP connections
    if (!listener) {
        fprintf(stderr, "Could not create a listener!\n");
        return 1;   
    }
    
    signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        fprintf(stderr, "Could not create a signal event!\n");
        return 1;   
    }
    
    event_base_dispatch(base);                          //  event dispatching loop
    
    evconnlistener_free(listener);                      //  disable and deallocate an evconnlistener
    event_base_free(base);                              //  deallocate all memory associated with an event_base, and free the base
    event_free(signal_event);
    
    return 0;
    
}



