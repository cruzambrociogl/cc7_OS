#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

int main() {
    char *msg = "Shared Memory Example";

    // ask the kernel for a shared memory segment of 128 bytes
    int shm_id = shmget(IPC_PRIVATE, 128, IPC_CREAT | 0666);
    if (shm_id < 0) {
        printf("shmget failed\n");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        printf("fork failed\n");
        return 1;
    }

    if (pid == 0) {
        // child attaches to the shared memory
        char *ptr = (char *)shmat(shm_id, NULL, 0);
        printf("Child Process: Read \"%s\"\n", ptr);
        shmdt(ptr);
    } else {
        // parent attaches and writes
        char *ptr = (char *)shmat(shm_id, NULL, 0);
        strcpy(ptr, msg);
        printf("Parent Process: Writing \"%s\"\n", msg);
        shmdt(ptr);

        waitpid(pid, NULL, 0);

        // cleanup: destroy the segment
        shmctl(shm_id, IPC_RMID, NULL);
    }

    return 0;
}
