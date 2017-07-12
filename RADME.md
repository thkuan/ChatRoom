# [Linux Socket Programming]: Chat room

## Single-Process, Synchronous I/O Multiplexing Server
1. Monitor a coming event by checking all client socket file descriptors
=> select(...), FD_ISSET(...), and so on

## Multi-Process, Concurrent Server 
[Hint]:
1. Create a child process and avoid zombie process
=> fork() and waitpid(...)
2. Create shared memory for shared data structures
=> mmap(...)
3. Inter-process communication
=> signal(...) and kill(...)
