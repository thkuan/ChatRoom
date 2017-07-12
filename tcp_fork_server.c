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
#include "server_ipc.h"

/* Store client infomation that shared among processes */
client_info_t *g_ptr_clients = NULL;
/* Count the number of clients that shared among processes */
int *g_ptr_client_cnt = NULL;

int main()
{
    int server_sockfd = 0, client_sockfd = 0;
    int is_set_sock = 0;
    int child_pid = -1, cchild_pid = -1;
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
    /* Allocate shared memory and initialize them in advance */
    g_ptr_clients = (client_info_t *) create_shared_memory(sizeof(client_info_t)*MAX_CLIENTS);
    g_ptr_client_cnt = (int *) create_shared_memory(sizeof(int));
    if (g_ptr_clients == NULL || g_ptr_client_cnt == NULL) {
        perror("[Server]: Could not create shared memory by mmap(). Error");
        return -1;
    }

    /* !!! THIS IS THE OLD WAY, IPv4 ONLY. HOW ABOUT IPv6? */
    /* Open a TCP socket (connection oriented socket) */
    if ((server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)  {
        perror("[Server]: Could not create server-side socket. Error");
        return -1;
    }
    printf("[Server]: Socket(sockfd = %d) created\n", server_sockfd);
//#if (SERVER_NONBLOCKING == 1)
    /* Make the socket non-blocking */
    //fcntl(server_sockfd, F_SETFL, O_NONBLOCK);
//#endif

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

    /*
     * Use SW interrupt to register a message_handler for 
     * inter-process communication
     */
    signal(SIGUSR1, message_handler);
    while (1) {
        /* Wait and accept connection */
        client_sockfd = accept(server_sockfd, (struct sockaddr *) &client, &addrlen);
        if (client_sockfd == -1) {
            perror("[Server]: Accept failed. Error");
            return -1;
        }
        printf("[Server]: Accept a client connection. Total clients: %d\n", atomic_add(g_ptr_client_cnt, 1));

        /* 
         * Fork a child process to serve a client, and call fork() 
         * twice and waitpid() to avoid zombie process 
         */
        if ((child_pid = fork()) < 0) {
            fprintf(stderr, "[Server]: fork() error\n");
            return -1;
        } else if (child_pid == 0) {
            /* Child process*/
            if ((cchild_pid = fork()) < 0) {
                fprintf(stderr, "[Server]: fork() error\n");
                return -1;
            } else if (cchild_pid == 0) {
                close(server_sockfd);
                /* Parallel executing unit: Grand child process serves a new client */
                //printf("Grand child process: self = %d, parent = %d\n", getpid(), getppid());
                /* <TODO> */
                /* Save client info in shared memory */
                save_client_info(client, client_sockfd, "(no name)");
                client_handler(client_sockfd);
                /* Clean client info in shared memory */
                clean_client_info(getpid());
                close(client_sockfd);
                return 0;
            }
            //printf("Child process: self = %d, parent = %d\n", getpid(), getppid());
            /* Child process kill itself immediately */
            return 0;
        }
        /* Wait for the child_pid to change state */
        waitpid(child_pid, NULL, 0);
        //printf("Parent process: self = %d\n", getpid());
        close(client_sockfd);
        /* Parallel executing unit: Parent process continues doing for loop */
    }

    /* Close serverm but never reach here */
    close(server_sockfd);
    return 0;
}
