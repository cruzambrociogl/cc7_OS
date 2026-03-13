#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    int fd[2]; // file descriptor two ends
    char buffer[128]; // message location
    char *msg = "Hello from Parent"; // message

    // is a syscall, asks the kernel to create a pipe
    if (pipe(fd) < 0) {
        printf("pipe failed\n");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        printf("fork failed\n");
        return 1;
    }

    if (pid == 0) {
        // child reads
        close(fd[1]);
        read(fd[0], buffer, sizeof(buffer));
        printf("Child Process: Received \"%s\"\n", buffer);
        close(fd[0]);
    } else {
        // parent writes
        close(fd[0]);
        printf("Parent Process: Writing \"%s\"\n", msg);
        write(fd[1], msg, strlen(msg) + 1);
        close(fd[1]);
        waitpid(pid, NULL, 0);
    }

    return 0;
}
