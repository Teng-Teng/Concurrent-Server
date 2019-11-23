#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

#define MAXSIZE 2048

/*  get a line of data ending with \r\n  */
int get_line(int cfd, char *buf, int size) {
    int i = 0;
    char c = '\0';
    int n;
    
    while ((i < size - 1) && (c != '\n')) {
        n = recv(cfd, &c, 1, 0);
        if (n > 0) {
            if (c == '\r') {
                n = recv(cfd, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                    n = recv(cfd, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        } 
        else
            c = '\n';
    }
    
    buf[i] = '\0';
    if (-1 == n)
        i = n;
    return i;
}

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

/*  accept connection request  */
void do_accept(int lfd, int epfd) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    cfd = accept(lfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (cfd == -1) {
        perror("accept error");
        exit(1);        
    }
    
    char client_ip[64] = {0};
	printf("new client IP: %s, port: %d, cfd: %d\n", 
                inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
                ntohs(client_addr.sin_port), cfd);              //  print client information(IP/PORT/cfd)
                
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);                                  //  set cfd to non-blocking                  
    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;                              //  use epoll as an edge-triggered (EPOLLET) interface with nonblocking file descriptors
    ev.data.fd = cfd;                                           //  cfd listen for read event
    
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);         //  add cfd to the red-black tree
    if (ret == -1) {
        perror("epoll_ctl add connection file descriptor error");
        exit(1);
    }
}

void disconnect(int cfd, int epfd) {
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
    if (ret != 0) {
        perror("epoll_ctl error");
        exit(1);
    }
    close(cfd);
}

/*  send server local files to the browser  */
void send_file(int cfd, const char *file) {
    int n = 0;
    char buf[1024];
    
    int fd = open(file, O_RDONLY);
    if (fd == -1) {
        //  send 404 error page to browser
        perror("open error");
        exit(1);        
    }
    
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        send(cfd, buf, n, 0);
        
    close(fd);
}

/**
 * send the HTTP response
 * @param  {cfd}  connection file descriptor  
 * @param  {error_no}  error number  
 * @param  {description}  error description  
 * @param  {type}  file type  
 * @param  {len}  file length  
 * @return {void}
*/
void send_response(int cfd, int error_no, char* description, char* type, int len) {
    char buf[1024] = {0};
    sprintf(buf, "HTTP/1.1 %d %s\r\n", error_no, description);
    sprintf(buf + strlen(buf), "%s\r\n", type);
    sprintf(buf + strlen(buf), "Content-Length:%d\r\n", len);
    send(cfd, buf, strlen(buf), 0);
    send(cfd, "\r\n", 2, 0);
}

/*  the server processes the request, sending back its response, providing a status code and appropriate data  */
void http_request(const char *file) {
    struct stat sbuf;
    
    int ret = stat(file, &sbuf);                                //  get file status and check if the file exists
    if (ret != 0) {
        //  send 404 error page to browser
        perror("stat error");
        exit(1);        
    }
    
    if (S_ISREG(sbuf.s_mode)) {                                 //  the file is a regular file
        send_response(cfd, 200, "OK", "Content-Type: text/plain; charset=iso-8859-1", sbuf.st_size);    //  send the HTTP response                                       //  send the HTTP response
        send_file(cfd, file);                                   //  send server local files to the browser
    }
}

/*  read data  */
void do_read(int cfd, int epfd) {
    char line[1024] = {0};
    int line = get_line(cfd, line, sizeof(line));
    if (line == 0) {
        printf("the client %d closed...\n", cfd);
        disconnect(cfd, epfd);
    } else {
        char method[16], path[256], protocol[16];
        sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
        printf("method = %s, path = %s, protocol = %s\n", method, path, protocol);
        
        while (1) {
            char buf[1024] = {0};
            line = get_line(cfd, buf, sizeof(buf));
            if (len == '\n')
                break;
            else if (len == -1)
                break;
        }
        
        if (strncasecmp(method, "GET", 3) == 0) {
            char *file = path + 1;                              //  get the file name the client wants to access
            http_request(cfd, file);
        }
    }
}

/*  use epoll to wait for an I/O event  */
void epoll_run(int port) {
    int i = 0;
    struct epoll_event all_events[MAXSIZE];
    
    int epfd = epoll_create(MAXSIZE);                            //  open an epoll file descriptor
    if (epfd == -1) {
        perror("epoll_create error");
        exit(1);        
    }
    
    int lfd = init_listen_fd(port, epfd);                       //  create listen file descriptor, add it to the red-black tree
    
    while (1) {
        int ret = epoll_wait(epfd, all_events, MAXSIZE, -1);     //  wait for an I/O event on an epoll file descriptor
        if (ret == -1) {
            perror("epoll_wait error");
            exit(1);
        }
            
        for (i = 0; i < ret; ++i) {
            struct epoll_event *pev = &all_events[i];           //  only handle read events, do not handle other events by default    

            if (!(pev->events & EPOLLIN))
                continue;
            if (pev->data.fd == lfd)
                do_accept(lfd, epfd);                           //  accept connection request
            else
                do_read(pev->data.fd, epfd);                    //  read data
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






