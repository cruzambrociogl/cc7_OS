# Lab 006 — Smart Parking Lot System (Semaphores)

## What It Does

Simulates a parking lot with **3 spaces** and **10 cars** (threads). Uses a **counting semaphore** to limit how many cars can park at once, and **mutexes** to protect shared data.

## How It Works

```
               10 Car Threads
            ┌───┬───┬───┬───┬───┐
            │ 0 │ 1 │ 2 │ 3 │...│
            └─┬─┴─┬─┴─┬─┴───┴───┘
              │   │   │
              ▼   ▼   ▼
        ┌─────────────────────┐
        │  SEMAPHORE (count=3) │ ◄── blocks when count reaches 0
        └──────────┬──────────┘
                   │
              ┌────┴────┐
              │  GATE   │
              └────┬────┘
                   │
        ┌──────────┴──────────┐
        │    PARKING LOT      │
        │  [  ] [  ] [  ]     │  ← only 3 spots
        └─────────────────────┘
```

## Step by Step

1. `main()` initializes the semaphore to 3 (available spots) and creates 2 mutexes
2. 10 car threads are created — they all start running immediately
3. Each car thread does:
   - **Arrive** — log arrival with timestamp (protected by `log_mutex`)
   - **Wait** — call `sem_wait()` — blocks if lot is full (count == 0)
   - **Park** — log how long it waited, update stats (protected by `stats_mutex`)
   - **Sleep** 1–5 seconds — simulates being parked
   - **Leave** — log departure, call `sem_post()` — frees a spot
4. `main()` waits for all threads to finish with `pthread_join()`
5. Print total cars parked and average wait time
6. All events are also written to `parking_log.log`

## Semaphore Timeline

```
Time ──────────────────────────────────────────────────────────────────────►

Semaphore: 3    2    1    0              1    0              1    0
                                         │                   │
Car 0:     [====== parked (2s) ======]   │                   │
Car 1:     [======== parked (3s) ========]                   │
Car 2:     [==== parked (2s) ====]       │                   │
Car 3:          waiting...          [=== parked (2s) ===]    │
Car 4:          waiting...               [== parked (1s) ==] │
Car 5:          waiting...                    waiting...     [== parked ==]
  ...

           ↑                        ↑
     3 cars enter immediately    Car 0 leaves → sem_post
     (sem count goes 3→2→1→0)    → Car 3 unblocks (sem_wait returns)
```

## Synchronization Overview

```
┌─────────────────────────────────────────────────────────────┐
│                        SHARED STATE                          │
│                                                              │
│  parking_semaphore (count=3) ─── controls lot capacity       │
│  log_mutex ──────────────────── protects printf/fprintf      │
│  stats_mutex ────────────────── protects total_parked and    │
│                                  total_wait_time             │
│  log_file ───────────────────── parking_log.log              │
└─────────────────────────────────────────────────────────────┘
          │              │              │
     ┌────┴───┐    ┌─────┴────┐   ┌────┴───┐
     │ Car 0  │    │  Car 1   │   │ Car 2  │  ...  (10 threads)
     │        │    │          │   │        │
     │ arrive │    │  arrive  │   │ arrive │
     │ wait   │    │  wait    │   │ wait   │
     │ park   │    │  park    │   │ park   │
     │ leave  │    │  leave   │   │ leave  │
     └────────┘    └──────────┘   └────────┘
```

## Mutex vs Semaphore

```
MUTEX (binary — 1 at a time):

  Thread A ──► [locked]  Thread B waits...
  Thread A ──► [unlock]  Thread B ──► [locked]

COUNTING SEMAPHORE (N at a time):

  count=3
  Thread A enters → count=2
  Thread B enters → count=1
  Thread C enters → count=0
  Thread D waits...  ← BLOCKED (count=0)
  Thread A leaves → count=1
  Thread D enters → count=0
```

## Race Condition Prevention

```
WITHOUT stats_mutex (BUG — lost update):

  Car 3 reads  total_parked = 5
  Car 7 reads  total_parked = 5      ← both read same value
  Car 3 writes total_parked = 6
  Car 7 writes total_parked = 6      ← should be 7!

WITH stats_mutex (CORRECT):

  Car 3 locks   stats_mutex
  Car 3 reads   total_parked = 5
  Car 3 writes  total_parked = 6
  Car 3 unlocks stats_mutex
  Car 7 locks   stats_mutex
  Car 7 reads   total_parked = 6     ← sees correct value
  Car 7 writes  total_parked = 7
  Car 7 unlocks stats_mutex
```

## Linux vs Windows

```
                        Linux/Mac                    Windows
                        ─────────                    ───────
Semaphore type:         sem_t* (POSIX named)         HANDLE (Win32)
Create semaphore:       sem_open()                   CreateSemaphore()
Wait (decrement):       sem_wait()                   WaitForSingleObject()
Signal (increment):     sem_post()                   ReleaseSemaphore()
Destroy:                sem_close()+sem_unlink()     CloseHandle()
Mutex type:             pthread_mutex_t              CRITICAL_SECTION
Lock:                   pthread_mutex_lock()         EnterCriticalSection()
Unlock:                 pthread_mutex_unlock()       LeaveCriticalSection()
Create thread:          pthread_create()             CreateThread()
Wait for threads:       pthread_join() loop          WaitForMultipleObjects()
Sleep:                  sleep(seconds)               Sleep(milliseconds)
Timer:                  clock_gettime()              QueryPerformanceCounter()
```

## How to Run

### Mac/Linux

```bash
make
./parking_lot
```

### Windows

```cmd
gcc -Wall -o parking_lot.exe main_windows.c
parking_lot.exe
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
[Thu Mar 19 16:20:06 2026] Car 2: Leaving parking lot
[Thu Mar 19 16:20:06 2026] Car 3: Parked successfully (waited 2.00 seconds)
...
[Thu Mar 19 16:20:17 2026] Car 9: Leaving parking lot

Total cars parked: 10
Average wait time: 3.50 seconds
```

## Project Structure

```
006Semaphores/
├── parking_lot.h         # Shared header (NUM_CARS, PARKING_SPACES, ParkingStats)
├── main_linux.c          # Mac/Linux version (pthreads + POSIX semaphores)
├── main_windows.c        # Windows version (WinAPI + Win32 semaphores)
├── Makefile              # Compiles the Mac/Linux version
├── parking_log.log       # Output log (generated at runtime)
├── 006Semaphores.pdf     # Lab assignment
├── LAB_DOCUMENTATION.md  # Detailed documentation
└── README.md             # This file
```
