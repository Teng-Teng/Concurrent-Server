/*  get a line of data ending with \r\n  */
int get_line(int cfd, char *buf, int size);

/*  get file type by file name  */
const char *get_file_type(const char *filename);

/*  convert hexadecimal to decimal  */
int hexit(char c);

/*  encode chinese ---> utf-8 chinese text(%23 %34 %5f)  */
void encode_str(char* to, int tosize, const char* from);

/*  decode utf-8 chinese text(%23 %34 %5f)--->chinese  */
void decode_str(char* to, char* from);

/*  error page  */
void send_error(int cfd, int status, char *title, char *text);

/*  create listen file descriptor, add it to the red-black tree  */
int init_listen_fd(int port, int epfd);

/*  accept connection request  */
void do_accept(int lfd, int epfd);

void disconnect(int cfd, int epfd);

/*  send server local files to the browser  */
void send_file(int cfd, const char *filename);

/*  send directory to the browser  */
void send_dir(int cfd, const char *dirname);

/**
 * send the HTTP response header
 * @param  {cfd}  connection file descriptor  
 * @param  {error_no}  error number  
 * @param  {description}  error description  
 * @param  {type}  file type  
 * @param  {len}  file length  
 * @return {void}
*/
void send_response_header(int cfd, int error_no, const char* description, const char* type, long len);

/*  the server processes the request, sending back its response, providing a status code and appropriate data  */
void http_request(const char *request, int cfd);

/*  read data  */
void do_read(int cfd, int epfd);

/*  use epoll to wait for an I/O event  */
void epoll_run(int port);


