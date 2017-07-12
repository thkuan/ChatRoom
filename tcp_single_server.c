#include <stdio.h>
#include <string.h>     // memset()
#include <sys/socket.h> // socket(), bind(), listen(), accept()
#include <netinet/in.h> // struct sockaddr_in, INADDR_ANY
#include <sys/wait.h>   // waitpid()
#include <arpa/inet.h>  // inet_ntop()
#include <unistd.h>     // fork(), close()
#include <netdb.h>      // struct addrinfo, getaddr_info(), freeaddrinfo()
#include <fcntl.h>      // fcntl()

/* Define macros here, or let server_config.h define macross */
#define MAX_CLIENTS (64)
#include "server_config.h"
#include "server_feature.h"

/* No synchronization technique should be adopted on handling global variables */
client_info_t g_clients[MAX_CLIENTS] = {0};

int g_client_cnt = 0;


int main()
{
    int server_sockfd = 0, client_sockfd = 0;
    int is_set_sock = 0;
    /* IPv4 only:
       struct sockaddr {
           unsigned short sa_family;
           char sa_data[14];
       };
       struct sockaddr_in { // Size = 16 B
           short int sin_family;
           unsigned short int sin_port;
           struct in_addr sin_addr;
           unsigned char sin_zero[8];
       };
       struct in_addr {
           uint32_t s_addr;
       };
    */
    struct sockaddr_in server, client;
    socklen_t addrlen = sizeof(client);

    /* !!! THIS IS THE OLD WAY, IPv4 ONLY. HOW ABOUT IPv6? */
    /* Open a TCP socket (connection oriented socket) */
    if ((server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)  {
        perror("[Server]: Could not create server-side socket. Error");
        return -1;
    }
    printf("[Server]: Socket(sockfd = %d) created\n", server_sockfd);
#if (SERVER_NONBLOCKING == 1)
    /* Make the socket non-blocking */
    //fcntl(server_sockfd, F_SETFL, O_NONBLOCK);
#endif

    /* Enable local address reuse */
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &is_set_sock, sizeof(is_set_sock)) < 0) {
        perror("[Server]: Enable local address reuse failed. Error");
        return -1;
    }
    printf("[Server]: Enable local address reuse succeed\n");

    /* Bind our local address/port so that the client can connect to us */
    server.sin_family = AF_INET;        // Internet domain sockets @ socket.h
    server.sin_addr.s_addr = INADDR_ANY;// IPv4 local host address
    server.sin_port = htons(SERVER_PORT);
    memset(server.sin_zero, '\0', sizeof(server.sin_zero));
    if (bind(server_sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server)) == -1) {
        perror("[Server]: Bind failed. Error");
        return -1;
    }
    printf("[Server]: Bind succeed\n");

    /*
     * Listen to the socket with backlog set to 10 which means 
     * 10 incomming connections are going to wait in the queue
     * until be accept()ed
     */
    if (listen(server_sockfd, BACKLOG) == -1) {
        perror("[Server]: Listen failed. Error");
        return -1;
    }
    printf("[Server]: Listen succeed\n");
    
    //show_ip("tw.yahoo.com");
    fd_set read_fds, active_fds;
    int fd, fd_max;
    //fd_max = getdtablesize();
    //printf("fd_max = %d, FD_SETSIZE = %d\n", fd_max, FD_SETSIZE);
    //struct timeval time_interval = {0};
    fd_max = server_sockfd;

    /* 
     * Initialize active_fds and read_fds, add server socket fd to let
     * the select() help detect a coming event.
     */
    FD_ZERO(&active_fds);
    FD_SET(server_sockfd, &active_fds);
    while (1) {
        /* Structure assignment, update client sock fd for monitoring */
        read_fds = active_fds;
        /* 
         * Select all fds from read_fds (server_sockfd/client_sockfd(s))
         * to be monitored with:
         * a) Blocking if (struct timeval *) timeout = NULL
         * b) Non-blocking if (struct timeval *) timeout = {0}, that occupies CPU
         */
        if (select(fd_max+1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("[Server]: select() error. Error");
            return -1;
        }
        for (fd = 0; fd <= fd_max; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == server_sockfd) {
                    /* The single server accepts a client onnection */
                    client_sockfd = accept(server_sockfd, (struct sockaddr *) &client, &addrlen);
                    if (client_sockfd == -1) {
                        perror("[Server]: Accept failed. Error");
                        return -1;
                    }
                    printf("[Server]: Accept a client(sockfd = %d) connection. Total clients: %d\n", client_sockfd, ++g_client_cnt);
                    /* Make the client_sockfd join active_fds */
                    FD_SET(client_sockfd, &active_fds);
                    /* Update fd_max for loop tracking */
                    if (client_sockfd > fd_max) {
                        fd_max = client_sockfd;
                    }
                } else {
                    /* The single server handles data from client */
                    char buf[MAX_RECEIVE_BYTES] = {'\0'};
                    int num_bytes;
                    /* Return the number of bytes actually read into the buf */
                    if ((num_bytes = recv(fd, buf, MAX_RECEIVE_BYTES-1, 0)) <= 0) {
                        if (num_bytes == 0) {
                            printf("[Server]: User %d lost connection. Total clients: %d\n", fd, --g_client_cnt);
                        } else {
                            perror("[Server]: recv() error. Error");
                        }
                        close(fd);
                        FD_CLR(fd, &active_fds);
                    /* Make the client_sockfd join active_fds */
                    } else {
                        /* Got some data from a client and broadcast to everyone except myself! */
                        for (int i = 0; i <= fd_max; i++) {
                            if (FD_ISSET(i, &active_fds)) {
                                if (i == server_sockfd) {
                                    printf("client(%d): %s", fd, buf);
                                } else if (i != fd && i != server_sockfd) {
                                    if (send(i, buf, num_bytes, 0) == -1) {
                                        perror("[Server]: send() error. Error");
                                    }
                                }
                            }
                        }
                    }
                } /* if (fd == server_sockfd) */
            } /* if (FD_ISSET(fd, &read_fds)) */
        } /* for (fd = 0; fd <= fd_max; fd++) */
    } /* while (1) */

    /* Close serverm but never reach here */
    close(server_sockfd);
    return 0;
}
