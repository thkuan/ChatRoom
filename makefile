CC = gcc
CFLAGS = -g -Wall -I.

#fork_server.out: tcp_fork_server.o server_feature.o server_ipc.o
#	$(CC) $(CFLAGS) -o fork_server.out tcp_fork_server.o server_feature.o server_ipc.o

single_server.out: tcp_single_server.o
	$(CC) $(CFLAGS) -o single_server.out tcp_single_server.o

clean:
	rm -f *.o *.out
