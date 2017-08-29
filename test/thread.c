#include <stdio.h>
#include <pthread.h>

int sum;
void *runner(void *param) {
    int i, upper = atoi(param);
    sum = 0;
    for (i = 1; i <= upper; i++) {
        sum += i;
    }
    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    pthread_attr_t attr;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <integer value>\n", argv[0]);
        return -1;
    }
    if (atoi(argv[1]) < 0) {
        fprintf(stderr, "%d must be >= 0\n", atoi(argv[1]));
        return -1;
    }

    // get the default attributes
    pthread_attr_init(&attr);
    // create the thread
    pthread_create(&tid, &attr, runner, argv[1]);
    // wait for the thread to exit
    pthread_join(tid, NULL);

    printf("sum = %d\n", sum);
    return 0;
}
