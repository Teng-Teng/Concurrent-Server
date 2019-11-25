int response_http(struct bufferevent *bev, const char *method, char *path);
const char *get_file_type(const char *filename);
int send_file_to_http(const char *filename, struct bufferevent *bev);
void send_header(struct bufferevent *bev, int status_code, const char* description, const char* type, long len);
int send_error(struct bufferevent *bev);
int send_dir(struct bufferevent *bev, const char *dirname);
void conn_readcb(struct bufferevent *bev, void *user_data);
void conn_eventcb(struct bufferevent *bev, short events,void *user_data);
void signal_cb(evutil_socket_t sig, short events,void *user_data);
void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data);
void strencode(char* to, int tosize, const char* from);
void strdecode(char* to, char* from);
int hexit(char c);


