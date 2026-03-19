#include <stdio.h>
#include <stdlib.h>
#include <windows.h>   /* Windows API — CreateThread, CreateSemaphore, WaitForSingleObject, etc. */
#include <time.h>      /* time(), ctime() for timestamps */
#include "parking_lot.h"

/*
 * Global synchronization primitives (Windows versions).
 *
 * parking_semaphore — Windows counting semaphore (HANDLE).
 *   Created with CreateSemaphore(NULL, initial_count, max_count, NULL).
 *   WaitForSingleObject decrements it (blocks if 0).
 *   ReleaseSemaphore increments it.
 *
 * log_mutex — CRITICAL_SECTION used to protect printf output.
 *   CRITICAL_SECTION is faster than a Windows Mutex object for
 *   same-process synchronization (no kernel transition needed).
 *
 * stats_mutex — CRITICAL_SECTION protecting the shared ParkingStats counters.
 */
HANDLE parking_semaphore;
CRITICAL_SECTION log_mutex;
CRITICAL_SECTION stats_mutex;

/* Shared statistics — updated by every car thread, read by main at the end */
ParkingStats stats = {0, 0.0};

/* Log file — all events are written here in addition to stdout */
FILE *log_file;

/*
 * log_event() — print a timestamped, thread-safe log message.
 *
 * Same logic as the Linux version, but uses CRITICAL_SECTION
 * instead of pthread_mutex_t.
 *
 * EnterCriticalSection = pthread_mutex_lock
 * LeaveCriticalSection = pthread_mutex_unlock
 */
void log_event(const char *message) {
    EnterCriticalSection(&log_mutex);

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[24] = '\0';  /* Remove trailing newline from ctime output */

    printf("[%s] %s\n", timestamp, message);
    fprintf(log_file, "[%s] %s\n", timestamp, message);

    LeaveCriticalSection(&log_mutex);
}

/*
 * car_thread() — the function each car thread runs.
 *
 * CreateThread requires signature: DWORD WINAPI (*)(LPVOID)
 *   - DWORD = unsigned 32-bit return value
 *   - WINAPI = Windows calling convention (__stdcall)
 *   - LPVOID = void* (generic pointer argument)
 *
 * Same lifecycle as the Linux version:
 *   1. Arrive  — log arrival, record current time
 *   2. Wait    — WaitForSingleObject blocks if lot is full
 *   3. Park    — log wait time, Sleep 1-5 seconds
 *   4. Leave   — log departure, ReleaseSemaphore frees the spot
 */
DWORD WINAPI car_thread(LPVOID arg) {
    int car_id = (int)(INT_PTR)arg;
    char message[256];

    /* --- Step 1: Arrive --- */
    sprintf(message, "Car %d: Arrived at parking lot", car_id);
    log_event(message);

    /*
     * Record time before waiting.
     * QueryPerformanceCounter gives high-resolution timestamps on Windows.
     * Used to measure how long the car waited for a spot.
     */
    LARGE_INTEGER freq, wait_start, wait_end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&wait_start);

    /* --- Step 2: Wait for a spot --- */
    /*
     * WaitForSingleObject(handle, timeout)
     *   - parking_semaphore = the semaphore to wait on
     *   - INFINITE = no timeout, wait forever
     *
     * If semaphore count > 0: decrements and returns immediately
     * If semaphore count == 0: blocks until ReleaseSemaphore is called
     *
     * This is the Windows equivalent of sem_wait() on Linux.
     */
    WaitForSingleObject(parking_semaphore, INFINITE);

    /* --- Step 3: Park --- */
    QueryPerformanceCounter(&wait_end);

    /* Calculate wait time: ticks_difference / ticks_per_second = seconds */
    double wait_time = (double)(wait_end.QuadPart - wait_start.QuadPart) / freq.QuadPart;

    sprintf(message, "Car %d: Parked successfully (waited %.2f seconds)", car_id, wait_time);
    log_event(message);

    /* Update shared statistics (protected by stats_mutex) */
    EnterCriticalSection(&stats_mutex);
    stats.total_parked++;
    stats.total_wait_time += wait_time;
    LeaveCriticalSection(&stats_mutex);

    /*
     * Simulate parking for 1-5 seconds.
     * Sleep() on Windows takes milliseconds, so multiply by 1000.
     */
    int park_duration = (rand() % 5) + 1;
    Sleep(park_duration * 1000);

    /* --- Step 4: Leave --- */
    sprintf(message, "Car %d: Leaving parking lot", car_id);
    log_event(message);

    /*
     * ReleaseSemaphore(semaphore, release_count, previous_count)
     *   - parking_semaphore = the semaphore to signal
     *   - 1 = increment count by 1 (free one spot)
     *   - NULL = we don't need the previous count
     *
     * If threads are blocked in WaitForSingleObject, one wakes up.
     * This is the Windows equivalent of sem_post() on Linux.
     */
    ReleaseSemaphore(parking_semaphore, 1, NULL);

    return 0;
}

/*
 * main() — set up synchronization, launch car threads, print results.
 *
 * Same flow as Linux version but with Windows API calls:
 *   - CreateSemaphore instead of sem_init
 *   - InitializeCriticalSection instead of PTHREAD_MUTEX_INITIALIZER
 *   - CreateThread instead of pthread_create
 *   - WaitForMultipleObjects instead of pthread_join loop
 *   - CloseHandle / DeleteCriticalSection for cleanup
 */
int main(void) {
    srand((unsigned int)time(NULL));

    /* Open log file for writing — all events will be recorded here */
    log_file = fopen("parking_log.log", "w");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file\n");
        return 1;
    }

    /*
     * CreateSemaphore(security, initial_count, max_count, name)
     *   - NULL = default security
     *   - PARKING_SPACES = initial available spots (3)
     *   - PARKING_SPACES = maximum count the semaphore can reach
     *   - NULL = unnamed semaphore (only used within this process)
     *
     * Unlike sem_init, CreateSemaphore returns a HANDLE.
     * If it fails, it returns NULL.
     */
    parking_semaphore = CreateSemaphore(NULL, PARKING_SPACES, PARKING_SPACES, NULL);
    if (parking_semaphore == NULL) {
        fprintf(stderr, "Failed to create semaphore\n");
        return 1;
    }

    /* Initialize critical sections (lightweight mutexes for same-process use) */
    InitializeCriticalSection(&log_mutex);
    InitializeCriticalSection(&stats_mutex);

    HANDLE threads[NUM_CARS];

    printf("Smart Parking Lot Simulation\n");
    printf("Parking spaces: %d | Cars: %d\n\n", PARKING_SPACES, NUM_CARS);

    /* Create all car threads */
    for (int i = 0; i < NUM_CARS; i++) {
        /*
         * CreateThread(security, stack_size, function, argument, flags, thread_id)
         *   - NULL = default security
         *   - 0 = default stack size
         *   - car_thread = function to run
         *   - (LPVOID)(INT_PTR)i = car ID passed as the argument
         *   - 0 = start immediately
         *   - NULL = we don't need the thread ID
         *
         * INT_PTR is used for the cast to avoid truncation warnings
         * on 64-bit Windows (where LPVOID is 8 bytes but int is 4).
         */
        threads[i] = CreateThread(NULL, 0, car_thread, (LPVOID)(INT_PTR)i, 0, NULL);
        if (threads[i] == NULL) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return 1;
        }
    }

    /*
     * WaitForMultipleObjects(count, handles, wait_all, timeout)
     *   - NUM_CARS = number of thread handles
     *   - threads = array of handles
     *   - TRUE = wait for ALL threads (not just any one)
     *   - INFINITE = no timeout
     *
     * This is more efficient than calling WaitForSingleObject in a loop.
     * After this returns, every car has arrived, parked, and left.
     */
    WaitForMultipleObjects(NUM_CARS, threads, TRUE, INFINITE);

    /* Close all thread handles (Windows requires explicit cleanup) */
    for (int i = 0; i < NUM_CARS; i++) {
        CloseHandle(threads[i]);
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
     * CloseHandle releases the semaphore's kernel object.
     * DeleteCriticalSection frees internal resources.
     */
    CloseHandle(parking_semaphore);
    DeleteCriticalSection(&log_mutex);
    DeleteCriticalSection(&stats_mutex);

    return 0;
}
