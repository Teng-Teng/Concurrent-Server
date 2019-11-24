#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <ctype.h>
#include "epoll_server.h"

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

/*  convert hexadecimal to decimal  */
int hexit(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
        
    return 0;
}

/*  encode chinese ---> utf-8 chinese text(%23 %34 %5f)  */
void encode_str(char* to, int tosize, const char* from) {
    int tolen;
    
    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0) {
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int)*from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}

/*  decode utf-8 chinese text(%23 %34 %5f)--->chinese  */
void decode_str(char* to, char* from) {
    for ( ; *from != '\0'; ++to, ++from) {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
            *to = hexit(from[1])*16 + hexit(from[2]);
            from += 2;
        }
        else
            *to = *from;
    }
    *to = '\0';
}

/*  error page  */
void send_error(int cfd, int status, char *title, char *text) {
    char buf[4096] = {0};
    
    sprintf(buf, "%s %d %s\r\n", "HTTP/1.1", status, title);
    sprintf(buf + strlen(buf), "Content-Type:%s\r\n", "text/html");
    sprintf(buf + strlen(buf), "Content-Length:%d\r\n", -1);
    sprintf(buf + strlen(buf), "Connection: close\r\n");
    send(cfd, buf, strlen(buf), 0);
    send(cfd, "\r\n", 2, 0);
    
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "<html><head><title>%d %s</title></head>\n", status, title);
    sprintf(buf + strlen(buf), "<body bgcolor=\"#cc99cc\"><h4 align=\"center\">%d %s</h4>\n", status, title);
    sprintf(buf + strlen(buf), "%s\n", text);
    sprintf(buf + strlen(buf), "<hr>\n</body>\n</html>\n");
    send(cfd, buf, strlen(buf), 0);
    
    return;
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
    
    int cfd = accept(lfd, (struct sockaddr *)&client_addr, &client_addr_len);
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
    if (ret == -1) {
        perror("epoll_ctl delete cfd error");
        exit(1);
    }
    close(cfd);
}

/*  send server local files to the browser  */
void send_file(int cfd, const char *filename) {
    int len = 0, ret;
    char buf[4096] = {0};
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        send_error(cfd, 404, "Not Found", "No such file or directory");         //  send 404 error page to browser
        perror("open error");
        exit(1);        
    }
    
    while ((len = read(fd, buf, sizeof(buf))) > 0) {
        ret = send(cfd, buf, len, 0);
        if (ret == -1) {
            printf("errno = %d\n", errno);
            if (errno == EAGAIN) {
                perror("send error");
                continue;
            } else if (errno == EINTR) {
                perror("send error");
                continue;
            } else {
                perror("send error");
                exit(1);
            }
        }   
        
        if (ret < 4096)
            printf("-----send ret: %d\n", ret);
    }
        
    close(fd);
}

/*  send directory to the browser  */
void send_dir(int cfd, const char *dirname) {
    int i, ret;
    char buf[4096] = {0};
    
    sprintf(buf, "<html><head><title>directory name: %s</title></head>", dirname);
    sprintf(buf + strlen(buf), "<body><h1>current directory: %s</h1><table>", dirname);

    char enstr[1024] = {0};
    char path[1024] = {0};
    
    struct dirent** ptr;
    int num = scandir(dirname, &ptr, NULL, alphasort);
    
    for (i = 0; i < num; ++i) {
        char* name = ptr[i]->d_name;
        
        sprintf(path, "%s/%s", dirname, name);
        printf("path = %s ============\n", path);
        struct stat st;
        stat(path, &st);
        
        encode_str(enstr, sizeof(enstr), name);
        
        if (S_ISREG(st.st_mode))
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>", enstr, name, (long)st.st_size);
        else if (S_ISDIR(st.st_mode))
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", enstr, name, (long)st.st_size);
    
        ret = send(cfd, buf, strlen(buf), 0);
        if (ret == -1) {
            printf("errno = %d\n", errno);
            if (errno == EAGAIN) {
                perror("send error");
                continue;
            } else if (errno == EINTR) {
                perror("send error");
                continue;
            } else {
                perror("send error");
                exit(1);
            }
        } 
        memset(buf, 0, sizeof(buf));
    }
    
    sprintf(buf + strlen(buf), "</table></body></html>");
    send(cfd, buf, strlen(buf), 0);
    printf("Directory information successfully sent!\n");
    
#if 0 
    DIR* dir = opendir(dirname);                                    //  open a directory
    if (dir == NULL) {
        perror("open dir error");
        exit(1);
    }
    
    struct dirent* ptr = NULL;
    while ((ptr = readdir(dir)) != NULL)
        char* name = ptr->d_name;
        
    closedir(dir);
#endif
}

/**
 * send the HTTP response header
 * @param  {cfd}  connection file descriptor  
 * @param  {error_no}  error number  
 * @param  {description}  error description  
 * @param  {type}  file type  
 * @param  {len}  file length  
 * @return {void}
*/
void send_response_header(int cfd, int error_no, const char* description, const char* type, long len) {
    char buf[1024] = {0};
    sprintf(buf, "HTTP/1.1 %d %s\r\n", error_no, description);      //  status line
    sprintf(buf + strlen(buf), "Content-Type:%s\r\n", type);        //  response fields
    sprintf(buf + strlen(buf), "Content-Length:%ld\r\n", len);
    send(cfd, buf, strlen(buf), 0);
    send(cfd, "\r\n", 2, 0);                                        //  blank line
}

/*  the server processes the request, sending back its response, providing a status code and appropriate data  */
void http_request(const char *request, int cfd) {
    char method[16], path[1024], protocol[16];
    sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
    printf("method = %s, path = %s, protocol = %s\n", method, path, protocol);
    
    decode_str(path, path);                                     //  decode utf-8 chinese text(%23 %34 %5f)--->chinese
    char *file = path + 1;                                      //  get the file name the client wants to access, path = /hello.c
    if (strcmp(path, "/") == 0)                                 //  if didn't specify a resource to access, display the contents of the resource directory by default
        file = "./";
    
    struct stat st;
    int ret = stat(file, &st);                                  //  get file status and check if the file exists
    if (ret == -1) {
        send_error(cfd, 404, "Not Found", "No such file or directory");         //  send 404 error page to browser
        perror("stat error");
        return;        
    }
    
    if (S_ISDIR(st.st_mode)) {                                   //  the file is a directory
        send_response_header(cfd, 200, "OK", get_file_type(".html"), -1);    //  send the HTTP response header                                      
        send_dir(cfd, file);
    } else if (S_ISREG(st.st_mode)) {                            //  the file is a regular file
        send_response_header(cfd, 200, "OK", get_file_type(file), st.st_size);                                        
        send_file(cfd, file);                                   //  send server local files to the browser
    }
}

/*  read data  */
void do_read(int cfd, int epfd) {
    char line[1024] = {0};
    int len = get_line(cfd, line, sizeof(line));                //  read the HTTP request line "GET /hello.c HTTP/1.1"
    
    if (len == 0) {
        printf("the client %d closed...\n", cfd);
        disconnect(cfd, epfd);
    } else {
        printf("================ the HTTP request header ================\n");
        printf("the HTTP request line: %s\n", line);
        
        while (len) {                                           //  read the rest of the data
            char buf[1024] = {0};
            len = get_line(cfd, buf, sizeof(buf));                
            printf("-----: %s\n", buf);
        }
        printf("======================== The End ========================\n");
        
        /*  
        while (1) {
            char buf[1024] = {0};
            len = get_line(cfd, buf, sizeof(buf));
            if (len == '\n')
                break;
            else if (len == -1)
                break;
        } 
        */
    }
    
    if (strncasecmp("GET", line, 3) == 0) {                   //  HTTP request line "GET /hello.c HTTP/1.1", check whether it's a get request
        http_request(line, cfd);
        disconnect(cfd, epfd);
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






