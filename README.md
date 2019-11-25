
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
    The letters you type will be capitalized and sent back to you.
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
    The letters you type will be capitalized and sent back to you.
```

# 3. Highly Performance I/O Server--Epoll Reactor Pattern
     Use epoll as an edge-triggered (EPOLLET) interface with nonblocking file descriptors.

# 4. Epoll HTTP Server

```bash
# Compile the program
    gcc epoll_server.c -o epoll_server -Wall -g 
# Start the server
    ./epoll_server SERVER_PORT SERVER_DIRECTORY
# Open the browser
    enter URL "127.0.0.1:SERVER_PORT/FILENAME" 
```

# 5. Libevent HTTP Server

```bash
# Compile the program
    gcc libevent_http.c main.c -o server -Wall -g -levent 
# Start the server
    ./server SERVER_PORT SERVER_DIRECTORY
# Open the browser
    enter URL "127.0.0.1:SERVER_PORT/FILENAME" 
```

# 6. Libevent Tcp Server

```bash
# Compile the program
    gcc server.c -o server -Wall -g -levent
    gcc client.c wrap.c -o client -Wall -g
# Start the server
    ./server
# Start the client
    ./client
    or using netcat to test
    nc 127.0.0.1 SERVER_PORT
    Anything you type will be echoed back to you.
```

<p align="center">
  <img src="https://libevent.org/libevent3.png" alt="libevent logo"/>
</p>



[![Appveyor Win32 Build Status](https://ci.appveyor.com/api/projects/status/ng3jg0uhy44mp7ik?svg=true)](https://ci.appveyor.com/project/libevent/libevent)
[![Travis Build Status](https://travis-ci.org/libevent/libevent.svg?branch=master)](https://travis-ci.org/libevent/libevent)
[![Coverage Status](https://coveralls.io/repos/github/libevent/libevent/badge.svg)](https://coveralls.io/github/libevent/libevent)
[![Join the chat at https://gitter.im/libevent/libevent](https://badges.gitter.im/libevent/libevent.svg)](https://gitter.im/libevent/libevent?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![doxygen](https://img.shields.io/badge/doxygen-documentation-blue.svg)](https://libevent.org/doc)



# 1. BUILDING AND INSTALLATION

## Autoconf

```
$ ./configure
$ make
$ make verify   # (optional)
$ sudo make install
```

See [Documentation/Building#Autoconf](/Documentation/Building.md#autoconf) for more information

## CMake (Windows)

Install CMake: <https://www.cmake.org>

```
$ md build && cd build
$ cmake -G "Visual Studio 10" ..   # Or use any generator you want to use. Run cmake --help for a list
$ cmake --build . --config Release # Or "start libevent.sln" and build with menu in Visual Studio.
```

See [Documentation/Building#Building on Windows](/Documentation/Building.md#building-on-windows) for more information

## CMake (Unix)

```
$ mkdir build && cd build
$ cmake ..     # Default to Unix Makefiles.
$ make
$ make verify  # (optional)
```

See [Documentation/Building#Building on Unix (With CMake)](/Documentation/Building.md#building-on-unix-cmake) for more information

# 2. USEFUL LINKS:

For the latest released version of Libevent, see the official website at
<http://libevent.org/> .

There's a pretty good work-in-progress manual up at
   <http://www.wangafu.net/~nickm/libevent-book/> .

For the latest development versions of Libevent, access our Git repository
via

```
$ git clone https://github.com/libevent/libevent.git
```

You can browse the git repository online at:

<https://github.com/libevent/libevent>

To report bugs, issues, or ask for new features:

__Patches__: https://github.com/libevent/libevent/pulls
> OK, those are not really _patches_. You fork, modify, and hit the "Create Pull Request" button.
> You can still submit normal git patches via the mailing list.

__Bugs, Features [RFC], and Issues__: https://github.com/libevent/libevent/issues
> Or you can do it via the mailing list.

There's also a libevent-users mailing list for talking about Libevent
use and development: 

<http://archives.seul.org/libevent/users/>

# 3. ACKNOWLEDGMENTS

The [following people](/CONTRIBUTORS.md) have helped with suggestions, ideas,
code or fixing bugs.

