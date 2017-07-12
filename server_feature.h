#ifndef SERVER_FEATURE
#define SERVER_FEATURE

#include <sys/socket.h> // struct sockaddr_in, socket(), bind(), listen(), accept()
#include <netinet/in.h>
#include "server_config.h"

#define MAX_RECEIVE_BYTES   (16)

typedef struct client_info_s {
    //struct sockaddr_in addr;
    int id;
    int pid;
    int sockfd;
    struct sockaddr_in addr;
    char name[CLIENT_NAME_LEN];
    char msg_buf[MAX_RECEIVE_BYTES];
} client_info_t;

/* Feature prototypes */
int show_ip(const char *ip_addr);
void save_client_info(struct sockaddr_in client, int client_sockfd, char *name);
void clean_client_info(int target_pid);
int atomic_add(int *ptr, int val);
int atomic_sub(int *ptr, int val);
void client_handler(int client_sockfd);
void message_handler(int unused_sig);

#endif
