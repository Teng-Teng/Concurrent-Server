# 1. Multiprocess Tcp Server

```bash
# Compile the program
    gcc server.c wrap.c -o server -Wall
# Start the server
    ./server
# Using netcat to test
    nc 127.0.0.1 SERVER_PORT
```

# 2. Multithreaded Tcp Server

```bash
# Compile the program
    gcc server.c wrap.c -o server -Wall -lpthread
# Start the server
    ./server
# Using netcat to test
    nc 127.0.0.1 SERVER_PORT
```
