# [Linux Socket Programming]: Chat room
[Reference]:
1) [Beej's Guide to Network Programming-Using Internet Sockets](http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html)
2) [http://www.cs.dartmouth.edu/~campbell/cs60/socketprogramming.html](http://www.cs.dartmouth.edu/~campbell/cs60/socketprogramming.html)
3) [http://speed.cis.nctu.edu.tw/~ydlin/course/cn/np/term-proj-spec-chatroom.pdf](http://speed.cis.nctu.edu.tw/~ydlin/course/cn/np/term-proj-spec-chatroom.pdf)

## TCP client-server
![TCP server/client](http://www.cs.dartmouth.edu/~campbell/cs60/TCPsockets.jpg)

## Single-Process, Synchronous I/O Multiplexing Server
1. Monitor a coming event by checking all client socket file descriptors
    - select(...), FD_ISSET(...), and so on

## Multi-Process, Concurrent Server
![](http://www.cs.dartmouth.edu/~campbell/cs60/concurrentserver.jpg)
[Hint]:
1. Create a child process and avoid zombie process
    - fork() and waitpid(...)
2. Create shared memory for shared data structures
    - mmap(...)
3. Inter-process communication
    - signal(...) and kill(...)
