#ifndef PARKING_LOT_H
#define PARKING_LOT_H

#define NUM_CARS 10        /* Total number of car threads to simulate */
#define PARKING_SPACES 3   /* Number of available parking spots (N) */

/*
 * ParkingStats — shared counters protected by stats_mutex.
 *
 * Every car thread updates these after successfully parking.
 * At the end of the simulation, main() reads them to compute
 * and display the average waiting time.
 */
typedef struct {
    int total_parked;        /* How many cars have parked so far */
    double total_wait_time;  /* Sum of all cars' wait times (seconds) */
} ParkingStats;

/*
 * log_event() — thread-safe timestamped logging.
 *
 * Locks the log mutex, prints a message with a timestamp in the format:
 *   [Fri Mar 21 13:38:46 2025] Car 3: Parked successfully (waited 0.00 seconds)
 *
 * The log_mutex pointer and the formatted message are passed in.
 * This function is platform-independent — the mutex type and locking
 * mechanism are handled in the platform-specific main files.
 *
 * Note: This is declared here but NOT defined in parking_lot.c because
 * the mutex type differs between Linux (pthread_mutex_t) and Windows
 * (CRITICAL_SECTION). Each platform's main file provides its own
 * implementation.
 */

#endif
