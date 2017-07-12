#ifndef SERVER_IPC
#define SERVER_IPC

int sem_get_access();
int sem_release_access();
void *create_shared_memory(int size);
void initialize_shared_memory(void *addr, int size);

#endif
