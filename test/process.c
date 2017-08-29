#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

int value = 0;
void *runner(void *param) {
    value = 5;
    pthread_exit(0);
}

int main()
{
    pid_t pid;
    pthread_t tid;
    pthread_attr_t attr;
    if ((pid = fork()) < 0) { // error occurred
        fprintf(stderr, "fork() failed\n");
        return -1;
    }
    else if (pid == 0) { // child process
        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, runner, NULL);
        pthread_join(tid, NULL);
        printf("Child: value = %d\n", value);
    }
    else { // parent process
        wait(NULL);
        printf("Parent: value = %d\n", value);
    }
    return 0;
}
