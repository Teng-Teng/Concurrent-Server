#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include "libevent_http.h"

#define _HTTP_CLOSE_ "Connection: close\r\n"

int response_http(struct bufferevent *bev, const char *method, char *path) {
    if (strcasecmp("GET", method) == 0) {
        strdecode(path, path);
        char *pf = &path[1];
        
        if (strcmp(path, "/") == 0 ||  strcmp(path, "/.") == 0)
            pf = "./";
            
        printf("***** HTTP request resource path = %s, pf = %s\n", path, pf);
        
        struct stat st;
        if (stat(pf, &st) < 0) {
            perror("open file error: ");
            send_error(bev);
            return -1;
        }
        
        if (S_ISDIR(st.st_mode)) {
            send_header(bev, 200, "OK", get_file_type(".html"), -1);
            send_dir(bev, pf);
        }
        else {
            send_header(bev, 200, "OK", get_file_type(pf), st.st_size);
            send_file_to_http(pf, bev);
        }
    }
    return 0;
}

/*  get file type by file name  */
const char *get_file_type(const char *filename) { 
    char* dot;
    dot = strrchr(filename, '.');                               //  returns a pointer to the last occurrence of the character c in the string 
    
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml"; 
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";
        
    return "text/plain; charset=utf-8";
}

int send_file_to_http(const char *filename, struct bufferevent *bev) {
    int fd = open(filename, O_RDONLY);
    int ret = 0;
    char buf[4096] = {0};
    
    while ((ret = read(fd, buf, sizeof(buf)))) {
        bufferevent_write(bev, buf, ret);
        memset(buf, 0, ret);
    }
    
    close(fd);
    return 0;
}

int send_header(struct bufferevent *bev, int status_code, const char* description, const char* type, long len) {
    char buf[256] = {0};
    
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status_code, description);       //  HTTP/1.1 200 
    bufferevent_write(bev, buf, strlen(buf));
    sprintf(buf, "Content-Type:%s\r\n", type);                          //  response fields
    bufferevent_write(bev, buf, strlen(buf));
    sprintf(buf, "Content-Length:%ld\r\n", len);
    bufferevent_write(bev, buf, strlen(buf));
    bufferevent_write(bev, _HTTP_CLOSE_, strlen(_HTTP_CLOSE_));    
    bufferevent_write(bev, "\r\n", 2);
}

int send_error(struct bufferevent *bev) {
    send_header(bev, 404, "File Not Found", "text/html", -1);
    send_file_to_http("404.html", bev);
    return 0;
}

int send_dir(struct bufferevent *bev, const char *dirname) {
    char encode_name[1024];
    char path[1024];
    char timestr[64];
    struct stat st;
    struct dirent **dirinfo;
    int i;
    char buf[4096] = {0};
    
    sprintf(buf, "<html><head><meta charset=\"utf-8\"><title>%s</title></head>", dirname);
    sprintf(buf + strlen(buf), "<body><h1>current directory: %s</h1><table>", dirname);
    
    int num = scandir(dirname, &dirinfo, NULL, alphasort);
    for (i = 0; i < num; ++i) {

    }
    
}

void conn_readcb(struct bufferevent *bev, void *user_data) {
    printf("*********** start calling conn_readcb function %s ***********\n", __FUNCTION__);
    char buf[4096] = {0};
    char method[50], path[4096], protocol[32];
    bufferevent_read(bev, buf, sizeof(buf));
    prinf("buf[%s]\n", buf);
    sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", method, path, protocol);
    printf("method = %s, path = %s, protocol = %s\n", method, path, protocol);
    
    if (strcasecmp(method, "GET") == 0)
        response_http(bev, method, path);
        
    printf("************************ end call %s ************************ \n", __FUNCTION__);
}

void conn_eventcb(struct bufferevent *bev, short events,void *user_data) {
    printf("*********** start calling conn_eventcb function %s ***********\n", __FUNCTION__);
    if (events & BEV_EVENT_EOF) 
        printf("connection closed\n");
    else if (events & BEV_EVENT_ERROR)
        printf("an error occurred while connecting: %s\n", strerror(errno));
        
    bufferevent_free(bev);
    printf("************************* end call %s ************************ \n", __FUNCTION__);
}

void signal_cb(evutil_socket_t sig, short events,void *user_data) {
    struct event_base *base = user_data;
    struct timeval delay = { 1, 0 };
    printf("catch an interrupt signal, exit the event loop in one second\n");
    event_base_loopexit(base, &delay);                                  //  exit the event loop after the specified time
}

void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data) {
    printf("*********** start calling listener_cb function %s ***********\n", __FUNCTION__);
    struct event_base *base = user_data;
    struct bufferevent *bev;
    printf("fd is %d\n", fd);
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);          //  create a new socket bufferevent over an existing socket
    if (!bev) {
        fprintf(stderr, "Error creating bufferevent!\n");
        event_base_loopbreak(base);                                         //  abort the active event_base_loop() immediately
        return;
    }
    
    bufferevent_flush(bev, EV_READ | EV_WRITE, BEV_NORMAL);                 //  triggers the bufferevent to produce more data if possible
    bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, NULL);          //  changes the callbacks for a bufferevent
    bufferevent_enable(bev, EV_READ | EV_WRITE);                            //  enable read bufferevent

    printf("************************ end call %s ************************ \n", __FUNCTION__);
}






