#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>      /* POSIX threads and mutexes */
#include <semaphore.h>    /* POSIX semaphores — sem_wait, sem_post, etc. */
#include <unistd.h>       /* sleep() */
#include <time.h>         /* time(), ctime(), clock_gettime() for timestamps and wait measurement */
#include <fcntl.h>        /* O_CREAT, O_EXCL — flags for sem_open (macOS) */
#include "parking_lot.h"

/*
 * Global synchronization primitives.
 *
 * parking_semaphore — counting semaphore initialized to PARKING_SPACES (3).
 *   Acts as a gate: each sem_wait() decrements the count (one less spot).
 *   When it reaches 0, the next sem_wait() blocks until a car leaves
 *   and calls sem_post() to increment it back.
 *
 *   We use sem_open() (named semaphore) instead of sem_init() (unnamed)
 *   because sem_init is deprecated on macOS. sem_open works on both
 *   Linux and macOS. The semaphore is stored as a pointer (sem_t*).
 *
 * log_mutex — protects printf output so log lines from different threads
 *   don't interleave and produce garbled text.
 *
 * stats_mutex — protects the shared ParkingStats counters so two threads
 *   don't update total_parked or total_wait_time at the same time
 *   (which could cause lost updates).
 */
sem_t *parking_semaphore;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Shared statistics — updated by every car thread, read by main at the end */
ParkingStats stats = {0, 0.0};

/* Log file — all events are written here in addition to stdout */
FILE *log_file;

/*
 * log_event() — print a timestamped, thread-safe log message.
 *
 * Locks log_mutex so only one thread prints at a time.
 * Uses time() + ctime() to get a human-readable timestamp like:
 *   "Fri Mar 21 13:38:46 2025\n"
 *
 * ctime returns a 26-character string ending with '\n' and '\0'.
 * We strip the '\n' (at position 24) so it fits on one line.
 *
 * Format: [Fri Mar 21 13:38:46 2025] Car 3: Arrived at parking lot
 */
void log_event(const char *message) {
    pthread_mutex_lock(&log_mutex);

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[24] = '\0';  /* Remove trailing newline from ctime output */

    printf("[%s] %s\n", timestamp, message);
    fprintf(log_file, "[%s] %s\n", timestamp, message);

    pthread_mutex_unlock(&log_mutex);
}

/*
 * car_thread() — the function each car thread runs.
 *
 * pthread_create requires signature: void* (*)(void*)
 * The argument is the car's ID number (cast from int to void*).
 *
 * Lifecycle of each car:
 *   1. Arrive  — log arrival, record the current time (start of wait)
 *   2. Wait    — sem_wait() blocks if lot is full (semaphore count == 0)
 *   3. Park    — log how long the wait was, sleep 1-5 seconds
 *   4. Leave   — log departure, sem_post() frees the spot
 *
 * The semaphore is the key mechanism here:
 *   - sem_wait() is like taking a parking ticket. If tickets remain
 *     (count > 0), you proceed immediately. If none left (count == 0),
 *     you block until someone returns a ticket.
 *   - sem_post() is like returning the ticket when you leave.
 */
void *car_thread(void *arg) {
    int car_id = (int)(long)arg;
    char message[256];

    /* --- Step 1: Arrive --- */
    sprintf(message, "Car %d: Arrived at parking lot", car_id);
    log_event(message);

    /*
     * Record the time BEFORE waiting on the semaphore.
     * After sem_wait returns, the difference tells us how long the car waited.
     *
     * CLOCK_MONOTONIC is used instead of CLOCK_REALTIME because it's not
     * affected by system time changes (like NTP adjustments).
     */
    struct timespec wait_start, wait_end;
    clock_gettime(CLOCK_MONOTONIC, &wait_start);

    /* --- Step 2: Wait for a spot --- */
    /*
     * sem_wait() decrements the semaphore:
     *   - If count > 0: decrements and returns immediately (spot available)
     *   - If count == 0: blocks the thread until another thread calls sem_post()
     *
     * This is the core of the parking lot simulation — the semaphore
     * naturally limits how many cars can be inside at once.
     */
    sem_wait(parking_semaphore);

    /* --- Step 3: Park --- */
    clock_gettime(CLOCK_MONOTONIC, &wait_end);

    /* Calculate wait time in seconds (with sub-second precision) */
    double wait_time = (wait_end.tv_sec - wait_start.tv_sec) +
                       (wait_end.tv_nsec - wait_start.tv_nsec) / 1e9;

    sprintf(message, "Car %d: Parked successfully (waited %.2f seconds)", car_id, wait_time);
    log_event(message);

    /* Update shared statistics (protected by stats_mutex) */
    pthread_mutex_lock(&stats_mutex);
    stats.total_parked++;
    stats.total_wait_time += wait_time;
    pthread_mutex_unlock(&stats_mutex);

    /*
     * Simulate the car being parked for 1-5 seconds.
     * rand() % 5 gives 0-4, so +1 gives 1-5.
     */
    int park_duration = (rand() % 5) + 1;
    sleep(park_duration);

    /* --- Step 4: Leave --- */
    sprintf(message, "Car %d: Leaving parking lot", car_id);
    log_event(message);

    /*
     * sem_post() increments the semaphore:
     *   - If threads are blocked in sem_wait(), one of them wakes up
     *   - If no threads are waiting, the count simply increases
     *
     * This is like driving out of the lot — the gate now lets one more car in.
     */
    sem_post(parking_semaphore);

    return NULL;
}

/*
 * main() — set up synchronization, launch car threads, print results.
 *
 * Steps:
 *   1. Seed random number generator
 *   2. Initialize the semaphore to PARKING_SPACES
 *   3. Create NUM_CARS threads (each runs car_thread)
 *   4. Wait for all threads to finish
 *   5. Print statistics
 *   6. Clean up
 */
int main(void) {
    /* Seed rand() so each run produces different parking durations */
    srand(time(NULL));

    /* Open log file for writing — all events will be recorded here */
    log_file = fopen("parking_log.log", "w");
    if (!log_file) {
        perror("Failed to open log file");
        return 1;
    }

    /*
     * sem_open(name, flags, mode, initial_value)
     *   - "/parking_sem" = unique name for the semaphore (must start with '/')
     *   - O_CREAT = create if it doesn't exist
     *   - 0644 = file permissions (owner read/write, group/others read)
     *   - PARKING_SPACES = initial count (3 spots available)
     *
     * sem_open returns a pointer (sem_t*) unlike sem_init which uses a local sem_t.
     * We first unlink any leftover semaphore from a previous run to ensure
     * it starts fresh with the correct initial value.
     */
    sem_unlink("/parking_sem");
    parking_semaphore = sem_open("/parking_sem", O_CREAT, 0644, PARKING_SPACES);
    if (parking_semaphore == SEM_FAILED) {
        perror("sem_open failed");
        return 1;
    }

    pthread_t threads[NUM_CARS];

    printf("Smart Parking Lot Simulation\n");
    printf("Parking spaces: %d | Cars: %d\n\n", PARKING_SPACES, NUM_CARS);

    /* Create all car threads — they start running immediately */
    for (int i = 0; i < NUM_CARS; i++) {
        /*
         * We cast the integer car ID to (void*) to pass it as the thread argument.
         * Inside car_thread, we cast it back: (int)(long)arg
         *
         * This avoids allocating memory just to pass a single integer.
         * The (long) intermediate cast avoids warnings on 64-bit systems
         * where sizeof(void*) > sizeof(int).
         */
        pthread_create(&threads[i], NULL, car_thread, (void *)(long)i);
    }

    /*
     * Wait for all car threads to finish.
     * pthread_join blocks until the specified thread exits.
     * After this loop, all cars have arrived, parked, and left.
     */
    for (int i = 0; i < NUM_CARS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* --- Print final statistics --- */
    printf("\nTotal cars parked: %d\n", stats.total_parked);
    fprintf(log_file, "\nTotal cars parked: %d\n", stats.total_parked);
    if (stats.total_parked > 0) {
        printf("Average wait time: %.2f seconds\n",
               stats.total_wait_time / stats.total_parked);
        fprintf(log_file, "Average wait time: %.2f seconds\n",
                stats.total_wait_time / stats.total_parked);
    }

    fclose(log_file);

    /* --- Cleanup --- */
    /*
     * sem_close closes this process's handle to the named semaphore.
     * sem_unlink removes the semaphore name from the system so it
     * doesn't persist after the program exits.
     * pthread_mutex_destroy frees mutex resources.
     *
     * With PTHREAD_MUTEX_INITIALIZER, destroy is technically optional on
     * most systems, but it's good practice to always clean up.
     */
    sem_close(parking_semaphore);
    sem_unlink("/parking_sem");
    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&stats_mutex);

    return 0;
}
