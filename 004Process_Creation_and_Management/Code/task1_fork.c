#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        printf("fork failed\n");
        return 1;
    }

    if (pid == 0) {
        // child
        sleep(1);
        printf("Child Process: PID=%d, Parent PID=%d\n", getpid(), getppid());
    } else {
        // parent
        printf("Parent Process: PID=%d\n", getpid());
        sleep(2);
    }

    return 0;
}
// BUILDING AND RUNNING
// ./build_and_run.sh task1_fork
// ./build_and_run.sh task2_sync
// ./build_and_run.sh task3_pipe
// ./build_and_run.sh task4_multi
// ./build_and_run.sh task5_shm