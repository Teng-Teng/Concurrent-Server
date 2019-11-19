
# 1. Multithreaded Tcp Server

```bash
# Compile the program
    gcc server.c wrap.c -o server -Wall -lpthread
    gcc client.c wrap.c -o client -Wall
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
    gcc server.c wrap.c -o server -Wall
    gcc client.c wrap.c -o client -Wall
# Start the server
    ./server
# Start the client
    ./client
  or using netcat to test
    nc 127.0.0.1 SERVER_PORT
```

# 3. Highly Performance I/O Server--Epoll Reactor Pattern
