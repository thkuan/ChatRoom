#include <stdio.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/mman.h>   // mmap()
#include <sys/socket.h> // struct sockaddr_in, socket(), bind(), listen(), accept()
#include "server_ipc.h"

static int g_sem_id;

int sem_get_access() {
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    /* P() */
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;

    if (semop(g_sem_id, &sem_b, 1) == -1) {
        return 0;
    }

    return 1;
}

int sem_release_access() {
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    /* V() */
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;

    if (semop(g_sem_id, &sem_b, 1) == -1) {
        return 0;
    }

    return 1;
}

void *create_shared_memory(int size) {
    void *ptr = NULL;
    if (size > 0) {
        /* Memory buffer will be redable and writable */
        int protection = PROT_READ | PROT_WRITE;
        /* 
         * The buffer will be shared, but anonymous,
         * so only this process and its children will be able to use it
         */
        int visibility = MAP_ANONYMOUS | MAP_SHARED;

        if ((ptr = mmap(NULL, size, protection, visibility, 0, 0)) == MAP_FAILED) {
            ptr = NULL;
        }
        initialize_shared_memory(ptr, size);
    }

    return ptr;
}

void initialize_shared_memory(void *addr, int size) {
    if (addr != NULL && size > 0) {
        memset(addr, '\0', size);
    }

    return ;
}
