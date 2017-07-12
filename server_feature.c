#include <stdio.h>
#include <string.h>
#include <sys/socket.h> // recv(), accept()
#include <arpa/inet.h>  // inet_ntop()
#include <netdb.h>      // struct addrinfo, getaddr_info(), freeaddrinfo()
#include <unistd.h>     // getpid(), dup2()
#include <signal.h>     // kill(), SIGUSR1
#include "server_feature.h"
#include "server_config.h"
#include "server_ipc.h"

extern client_info_t *g_ptr_clients;
extern int *g_ptr_client_cnt;

//show_ip("tw.yahoo.com");
/* Feature showip */
int show_ip(const char *ip_addr) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN] = {'\0'};

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo(ip_addr, NULL, &hints, &res)) != 0) {
        perror("getaddrinfo failed. Error");
        return -1;
    }
   
    printf("IP addresses for %s:\n", ip_addr);

    /*
       struct addrinfo {
           int ai_flags;
           int ai_family;
           int ai_socktype;
           int ai_protocol;
           size_t ai_addrlen;
           struct sockaddr *ai_addr;
           cahr *ai_canonname;
           struct addrinfo *ai_next;
       };
     */
    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;

        /* 
         * Get the pointer to the address itself,
         * different fields in IPv4 or IPv6:
         */
        if (p->ai_family == AF_INET) {
            /* IPv4 */
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else {
            /* IPv6 */
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        /* Covert the IP to a string */
        inet_ntop(p->ai_family, addr, ipstr, (socklen_t) sizeof(ipstr));
        printf("%s\n", ipstr);
    }

    freeaddrinfo(res);
    return 0;
}

void save_client_info(struct sockaddr_in client, int client_sockfd, char *name) {
    client_info_t *ptr = NULL;
    int name_len = strlen(name);
    int i = 0;

    /* Search empty buffer by pid */
    for (i = 0; i < MAX_CLIENTS; i++) {
        if ((g_ptr_clients + i)->pid == 0) {
            ptr = g_ptr_clients + i;
            break;
        }
    }

    /* Save info to the buffer */
    ptr->id = i;
    ptr->pid = getpid();
    ptr->sockfd = client_sockfd;
    ptr->addr = client;
    if (name_len >= CLIENT_NAME_LEN) {
        strncpy(ptr->name, name, CLIENT_NAME_LEN);
        ptr->name[CLIENT_NAME_LEN-1] = '\0';
    } else {
        strncpy(ptr->name, name, name_len);
        ptr->name[name_len+1] = '\0';
    }

    return ;
}

void clean_client_info(int target_pid) {
    client_info_t *ptr = NULL;
    int i = 0;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if ((g_ptr_clients + i)->pid == target_pid) {
            ptr = g_ptr_clients + i;
            break;
        }
    }

    initialize_shared_memory(ptr, sizeof(client_info_t));

    return ;
}

int atomic_add(int *ptr, int val) {
    sem_get_access();
    *ptr += val;
    sem_release_access();

    return *ptr;
}

int atomic_sub(int *ptr, int val) {
    sem_get_access();
    *ptr -= val;
    sem_release_access();

    return *ptr;
}

void client_handler(int client_sockfd) {
    char buf[MAX_RECEIVE_BYTES] = {'\0'};
    int num_bytes = 0;

    while(1) {
        /* <TODO> Fix the behavior if input is larger than the macro */
        if ((num_bytes = recv(client_sockfd, buf, MAX_RECEIVE_BYTES - 1, 0)) == -1) {
//#if (SERVER_NONBLOCKING == 1)
            /* Do nothing if non-blocking */
//#else
            perror("[Server]: recv() error. Error");
            break;
//#endif
        } else if (num_bytes == 0) {
            printf("[Server]: User lost connection. Total clients: %d\n", atomic_sub(g_ptr_client_cnt, 1));
            break;
        } else if (num_bytes > 0) {
            int i = 0;
            /* Output recv()ed content to server stdout */
            fprintf(stdout, "%d B, %s", num_bytes, buf);
            /* Output recv()ed content to other clients' stdout */
            for (i = 0; i < MAX_CLIENTS; i++) {
                if ((g_ptr_clients + i)->pid != 0 && (g_ptr_clients + i)->pid != getpid()) {

                    sem_get_access();
                    memcpy((g_ptr_clients + i)->msg_buf, buf, MAX_RECEIVE_BYTES - 1);
                    (g_ptr_clients + i)->msg_buf[MAX_RECEIVE_BYTES-1] = '\0';
                    sem_release_access();
                    /* Notify specified fork()ed process to call message_handler() */
                    kill((g_ptr_clients + i)->pid, SIGUSR1);
                }
            }
            memset(buf, '\0', MAX_RECEIVE_BYTES);
        }
    }

    return ;
}

void message_handler(int unused_sig) {
    int i = 0;
   
    /* Once specified process got a notify, send msg to the client to whom is being served */
    for (i = 0; i < MAX_CLIENTS; i++) {
        if ((g_ptr_clients + i)->pid == getpid()) {

            sem_get_access();
            if (send((g_ptr_clients + i)->sockfd, (g_ptr_clients + i)->msg_buf, sizeof((g_ptr_clients + i)->msg_buf), 0) == -1) {
                perror("[Server]: send() error. Error");
            }
            sem_release_access();
            break;
        }
    }

    return ;
}
