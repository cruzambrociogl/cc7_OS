#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("Parent Process: PID=%d\n", getpid());

    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            printf("fork failed\n");
            return 1;
        }

        if (pid == 0) {
            printf("Child %d: PID=%d, Parent PID=%d\n", i + 1, getpid(), getppid());
            exit(0);
        }
    }

    // parent waits for all 3
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }

    return 0;
}
