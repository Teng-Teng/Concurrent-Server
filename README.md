
# 1. Multithreaded Tcp Server

```bash
# Compile the program
    gcc server.c wrap.c -o server -Wall -g -lpthread
    gcc client.c wrap.c -o client -Wall -g
# Start the server
    ./server
# Start the client
    ./client
  or using netcat to test
    nc 127.0.0.1 SERVER_PORT
```

# 2. I/O Multiplexing Server--Epoll

```bash
# Compile the program
    gcc server.c wrap.c -o server -Wall -g
    gcc client.c wrap.c -o client -Wall -g
# Start the server
    ./server
# Start the client
    ./client
  or using netcat to test
    nc 127.0.0.1 SERVER_PORT
```

# 3. Highly Performance I/O Server--Epoll Reactor Pattern
  Use epoll as an edge-triggered (EPOLLET) interface with nonblocking file descriptors.

