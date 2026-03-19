# Lab 006 — Semaphores (Smart Parking Lot System)

## Overview

This lab demonstrates how to use **semaphores** and **mutexes** to coordinate access to a limited shared resource. Multiple car threads compete for a fixed number of parking spaces. The semaphore acts as a gate that blocks cars when the lot is full, and lets them in as spots become available.

## What the Program Does

Simulates a parking lot with **3 spaces** and **10 cars** (threads). Each car:

1. **Arrives** at the lot and logs its arrival
2. **Waits** for an available spot (blocks if lot is full)
3. **Parks** for 1–5 seconds (random) and logs how long it waited
4. **Leaves** and frees the spot for the next car

At the end, the program reports:
- **Total cars parked** (should always be 10)
- **Average wait time** across all cars

All events are logged to both the terminal and a `parking_log.log` file.

## Real-World Analogy

This models parking systems in airports, shopping centers, and smart cities — anywhere that parking capacity is limited and concurrent access needs coordination:

```
           ┌───────────────────────────────────┐
           │         PARKING LOT (N=3)          │
           │                                    │
           │   [Spot 1]   [Spot 2]   [Spot 3]  │
           │                                    │
           └──────────────┬─────────────────────┘
                          │
                     ┌────┴────┐
                     │  GATE   │  ← semaphore (count = available spots)
                     └────┬────┘
                          │
              ┌───┬───┬───┼───┬───┬───┬───┐
              ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼
             Car Car Car Car Car Car Car  ...
              0   1   2   3   4   5   6
                          ↑
                    waiting in line
```

When the gate (semaphore) count reaches 0, no more cars can enter — they block until someone leaves.

## Project Structure

```
006Semaphores/
├── parking_lot.h        # Shared header — defines NUM_CARS, PARKING_SPACES, ParkingStats
├── main_linux.c         # Main program for Mac/Linux (pthreads + POSIX semaphores)
├── main_windows.c       # Main program for Windows (WinAPI threads + Win32 semaphores)
├── Makefile             # Compiles the Mac/Linux version
├── parking_log.log      # Generated output log (created at runtime)
├── 006Semaphores.pdf    # Lab assignment PDF
└── LAB_DOCUMENTATION.md # This file
```

## Architecture

### Synchronization Primitives

The program uses three synchronization primitives:

| Primitive | Type | Purpose |
|---|---|---|
| `parking_semaphore` | Counting semaphore (init=3) | Limits how many cars can be inside at once |
| `log_mutex` | Mutex | Protects printf/fprintf output from interleaving |
| `stats_mutex` | Mutex | Protects shared counters (total_parked, total_wait_time) |

### Why a Semaphore Instead of a Mutex?

A **mutex** is binary — it only allows one thread at a time (locked/unlocked).

A **counting semaphore** can allow **N** threads at a time:
- Initialized to N (number of parking spaces)
- Each `sem_wait()` decrements the count (one car enters)
- Each `sem_post()` increments the count (one car leaves)
- When count reaches 0, the next `sem_wait()` **blocks** until a `sem_post()` happens

```
Semaphore count over time (N=3, 10 cars):

Count: 3 ─┐
           ├─ Car 0 enters (sem_wait) → count = 2
           ├─ Car 1 enters (sem_wait) → count = 1
           ├─ Car 2 enters (sem_wait) → count = 0
           │
           │  Cars 3-9 BLOCK here (count = 0, no spots)
           │
           ├─ Car 0 leaves (sem_post) → count = 1
           ├─ Car 3 wakes up, enters  → count = 0
           │
           │  Cars 4-9 still blocking...
           │
           ├─ Car 1 leaves (sem_post) → count = 1
           ├─ Car 4 wakes up, enters  → count = 0
           ...and so on until all 10 cars have parked
```

### Car Thread Lifecycle

```
┌─────────────────────────────────────────────────┐
│                  car_thread(id)                   │
│                                                   │
│  1. Log "Arrived at parking lot"                  │
│         │                                         │
│  2. Record start time                             │
│         │                                         │
│  3. sem_wait(parking_semaphore)  ◄── may BLOCK    │
│         │                            here if      │
│         │                            lot is full  │
│  4. Record end time                               │
│     wait_time = end - start                       │
│         │                                         │
│  5. Log "Parked successfully (waited X seconds)"  │
│         │                                         │
│  6. Update stats (lock stats_mutex)               │
│         │                                         │
│  7. sleep(1-5 seconds)  ← simulates parking       │
│         │                                         │
│  8. Log "Leaving parking lot"                     │
│         │                                         │
│  9. sem_post(parking_semaphore)  ← frees spot     │
└─────────────────────────────────────────────────┘
```

### Thread-Safe Logging

Without the `log_mutex`, two threads printing at the same time could produce garbled output:

```
WITHOUT mutex (race condition):
  [Thu Mar 21 13:38:46 2025] Car [Thu Mar 21 13:38:46 2025] Car 1: Arrived
  0: Arrived

WITH mutex (correct):
  [Thu Mar 21 13:38:46 2025] Car 0: Arrived at parking lot
  [Thu Mar 21 13:38:46 2025] Car 1: Arrived at parking lot
```

The mutex ensures only one thread can execute the print block at a time.

### Statistics Tracking

Two shared counters are protected by `stats_mutex`:

```c
stats.total_parked++;           // incremented by each car after parking
stats.total_wait_time += wait;  // accumulated wait time
```

Without the mutex, two threads incrementing `total_parked` at the same time could cause a **lost update**:

```
Thread A reads total_parked = 5
Thread B reads total_parked = 5
Thread A writes total_parked = 6
Thread B writes total_parked = 6  ← should be 7!
```

## Platform Differences

| Aspect | Mac/Linux (`main_linux.c`) | Windows (`main_windows.c`) |
|---|---|---|
| Semaphore type | `sem_t*` (POSIX named) | `HANDLE` (Win32) |
| Semaphore create | `sem_open()` | `CreateSemaphore()` |
| Semaphore wait | `sem_wait()` | `WaitForSingleObject()` |
| Semaphore signal | `sem_post()` | `ReleaseSemaphore()` |
| Semaphore cleanup | `sem_close()` + `sem_unlink()` | `CloseHandle()` |
| Mutex type | `pthread_mutex_t` | `CRITICAL_SECTION` |
| Mutex lock | `pthread_mutex_lock()` | `EnterCriticalSection()` |
| Mutex unlock | `pthread_mutex_unlock()` | `LeaveCriticalSection()` |
| Thread creation | `pthread_create()` | `CreateThread()` |
| Thread waiting | `pthread_join()` loop | `WaitForMultipleObjects()` |
| Timer | `clock_gettime(CLOCK_MONOTONIC)` | `QueryPerformanceCounter()` |
| Sleep | `sleep(seconds)` | `Sleep(milliseconds)` |

### macOS Note

On macOS, `sem_init()` (unnamed semaphores) is **deprecated**. The Linux version uses `sem_open()` (named semaphores) instead, which works on both Linux and macOS without warnings.

## How to Run

### Mac/Linux

```bash
make
./parking_lot
```

### Windows (Command Prompt)

```cmd
gcc -Wall -o parking_lot.exe main_windows.c
parking_lot.exe
```

### Windows (Git Bash / MINGW64)

```bash
gcc -Wall -o parking_lot.exe main_windows.c
./parking_lot.exe
```

## Sample Output

```
Smart Parking Lot Simulation
Parking spaces: 3 | Cars: 10

[Thu Mar 19 16:20:04 2026] Car 0: Arrived at parking lot
[Thu Mar 19 16:20:04 2026] Car 1: Arrived at parking lot
[Thu Mar 19 16:20:04 2026] Car 2: Arrived at parking lot
[Thu Mar 19 16:20:04 2026] Car 0: Parked successfully (waited 0.00 seconds)
[Thu Mar 19 16:20:04 2026] Car 1: Parked successfully (waited 0.00 seconds)
[Thu Mar 19 16:20:04 2026] Car 2: Parked successfully (waited 0.00 seconds)
[Thu Mar 19 16:20:04 2026] Car 3: Arrived at parking lot
[Thu Mar 19 16:20:04 2026] Car 4: Arrived at parking lot
...
[Thu Mar 19 16:20:17 2026] Car 9: Leaving parking lot

Total cars parked: 10
Average wait time: 3.50 seconds
```

Output is also written to `parking_log.log`.

## Key Concepts Demonstrated

1. **Counting semaphores** — limiting concurrent access to N resources (not just 1)
2. **Mutex locks** — protecting shared data (log output and counters) from race conditions
3. **Thread synchronization** — coordinating 10 threads competing for 3 resources
4. **Wait time measurement** — timing how long each thread blocked on the semaphore
5. **Thread-safe file I/O** — writing to a log file from multiple threads without corruption
6. **Cross-platform threading** — POSIX (pthreads/sem_open) vs WinAPI (CreateThread/CreateSemaphore)

## Difference from Lab 005

| Aspect | Lab 005 (Threads) | Lab 006 (Semaphores) |
|---|---|---|
| Problem | Process data in parallel | Coordinate access to limited resource |
| Synchronization | None (thread-local data) | Semaphore + 2 mutexes |
| Threads do | Independent work (no sharing) | Compete for shared resource |
| Key mechanism | Work partitioning + merge | Semaphore as a gate/counter |
| Focus | Parallelism (speed) | Synchronization (correctness) |
