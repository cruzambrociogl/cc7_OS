#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        printf("fork failed\n");
        return 1;
    }

    if (pid == 0) {
        // child
        printf("Child Process: PID=%d, Parent PID=%d\n", getpid(), getppid());
    } else {
        // parent waits for child to finish
        waitpid(pid, NULL, 0);
        printf("Parent Process: Child has finished execution.\n");
    }

    return 0;
}
