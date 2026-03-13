# Lab 005 — Threads (Multi-Threaded Web Log Analyzer)

## Overview

This lab demonstrates how to use **multi-threading** to process large text-based log files efficiently. A web server generates log files containing HTTP request records, and our program analyzes them in parallel using threads.

## What the Program Does

Reads an `access.log` file and produces three results:

1. **Total Unique IPs** — how many distinct IP addresses made requests
2. **Most Visited URL** — which URL was accessed the most
3. **HTTP Errors** — total count of status codes between 400–599

It runs the analysis **twice** (single-threaded then multi-threaded) to compare execution times.

## Log Format

Each line in `access.log` follows this format:

```
IP - - [TIMESTAMP] "METHOD URL" STATUS
```

Example:

```
192.168.1.5 - - [10/Feb/2024:10:20:30] "POST /contact" 500
192.168.1.3 - - [10/Feb/2024:10:20:31] "DELETE /login" 403
192.168.1.4 - - [10/Feb/2024:10:20:32] "GET /about.html" 404
```

## Project Structure

```
005Threads/
├── log_processor.h              # Header file — data structures and function prototypes
├── log_processor.c              # Shared logic — hash table, parsing, chunk processing
├── main_linux.c                 # Main program for Mac/Linux (pthreads)
├── main_windows.c               # Main program for Windows (WinAPI threads)
├── Makefile                     # Compiles the Mac/Linux version
├── run.sh                       # Shell script to compile and run on Mac/Linux
├── run.bat                      # Batch script to compile and run on Windows
├── access.log                   # Sample log file (5000 entries)
├── access_log_file_generator.py # Python script to generate access.log
└── LAB_DOCUMENTATION.md         # This file
```

## Architecture

### Threading Approach: Local Data Per Thread (No Locks)

Each thread gets its **own private** hash tables and error counter. Threads never write to shared data, so **no mutexes are needed** during processing. After all threads finish, the main thread merges the results.

```
access.log (N lines)
       │
       ▼
  Main thread reads all lines
  Divides into chunks
       │
  ┌────┼────┬────┐
  ▼    ▼    ▼    ▼
 T1   T2   T3   T4     ← each processes ~N/4 lines
  │    │    │    │
  ▼    ▼    ▼    ▼
local local local local ← each has its own IP table, URL table, error count
  │    │    │    │
  └────┴────┴────┘
       │
       ▼
  Main thread merges all results
       │
       ▼
  Print output
```

**Why this approach over shared data + mutex?**

- Threads run at full speed with no waiting
- No risk of race conditions
- The merge step at the end is simple and fast
- This is what the lab PDF hints at: "Merge results from all threads"

### Data Structures

**Hash Table** (array of 256 buckets with linked list chaining):

- Used to count occurrences of IPs and URLs
- `hash_table_insert(table, key)` — increments count for that key (or creates it with count 1)
- `hash_table_merge(dest, src)` — adds all counts from src into dest
- `hash_table_count_keys(table)` — returns number of unique keys
- `hash_table_most_frequent(table, ...)` — finds the key with highest count

### Key Functions in log_processor.c

| Function | Purpose |
|---|---|
| `hash_table_create()` | Allocates a new empty hash table (using `calloc`) |
| `hash_table_insert()` | Adds/increments a key in the table (wraps `hash_table_insert_count`) |
| `hash_table_insert_count()` | (static) Adds a key with a given count — shared by `insert` and `merge` |
| `hash_table_merge()` | Combines two tables (reuses `hash_table_insert_count` internally) |
| `hash_table_count_keys()` | Counts unique keys (for unique IP count) |
| `hash_table_most_frequent()` | Finds key with max count (for most visited URL) |
| `hash_table_free()` | Frees all memory used by a table |
| `parse_log_line()` | Extracts IP, URL, status from a log line using sscanf |
| `process_chunk()` | Worker function — each thread calls this on its assigned lines |
| `read_log_file()` | Reads entire file into an array of strings |
| `free_lines()` | Frees the array of strings |

## Platform Differences

| Aspect | Mac/Linux (`main_linux.c`) | Windows (`main_windows.c`) |
|---|---|---|
| Thread creation | `pthread_create()` | `CreateThread()` |
| Thread waiting | `pthread_join()` | `WaitForMultipleObjects()` |
| Thread cleanup | automatic | `CloseHandle()` |
| Timer | `clock_gettime(CLOCK_MONOTONIC)` | `QueryPerformanceCounter()` |
| Compile flag | `-pthread` | none needed |
| Shared logic | `log_processor.c` (same) | `log_processor.c` (same) |

## How to Run

### Mac/Linux

```bash
# Option 1: Use the script
bash run.sh

# Option 2: Manual
make
./log_analyzer access.log 4
```

### Windows (Command Prompt)

```cmd
:: Option 1: Use the script
run.bat

:: Option 2: Manual
gcc -Wall -o log_analyzer.exe main_windows.c log_processor.c
log_analyzer.exe access.log 4
```

### Windows (Git Bash / MINGW64)

If you are using Git Bash instead of Command Prompt, note two differences:

1. **Run the executable with `./` prefix** — Git Bash does not search the current directory by default:

```bash
gcc -Wall -o log_analyzer.exe main_windows.c log_processor.c
./log_analyzer.exe access.log 4
```

2. **`.bat` scripts do not work in Git Bash** — run the commands manually instead:

```bash
gcc -Wall -o log_analyzer.exe main_windows.c log_processor.c
./log_analyzer.exe access.log 4
```

### Command-Line Arguments

```
./log_analyzer [filename] [num_threads]
```

- `filename` — path to log file (default: `access.log`)
- `num_threads` — number of threads to use (default: 4)

Examples:

```bash
./log_analyzer access.log 1    # single-threaded only
./log_analyzer access.log 8    # compare 1 thread vs 8 threads
./log_analyzer large.log 4     # use a different log file
```

## Sample Output

```
Reading log file: access.log
Total lines: 5000

--- Single-Threaded ---
Threads: 1
Total Unique IPs: 5
Most Visited URL: /login (873 times)
HTTP Errors: 3708
Execution Time: 0.0009 seconds

--- Multi-Threaded ---
Threads: 4
Total Unique IPs: 5
Most Visited URL: /login (873 times)
HTTP Errors: 3708
Execution Time: 0.0006 seconds
```

## Performance Comparison (1,000,000 lines)

| Run | Time | Speedup |
|---|---|---|
| Single-threaded (1 thread) | 0.1570s | — |
| Multi-threaded (4 threads) | 0.0484s | ~3.2x faster |

To generate a large log file for testing:

```bash
# Edit access_log_file_generator.py, change range(5000) to range(1000000)
python3 access_log_file_generator.py
```

## Changes & Optimizations

These simplifications were applied after the initial implementation. All changes keep the code simple and basic — no fancy additions.

1. **`hash_table_create()` uses `calloc` instead of `malloc` + loop** — `calloc` zero-initializes memory, so there is no need for a manual loop setting each bucket to `NULL`. Same result, less code.

2. **`hash_table_merge()` reuses insert logic** — previously, `merge` had its own copy of the bucket search and node creation code (~20 lines). Now it calls a shared `hash_table_insert_count()` helper, reducing it to ~7 lines. `hash_table_insert()` also wraps this same helper with count=1.

3. **Removed unused `hash_table_get()`** — was declared in the header and implemented but never called anywhere. Removed from both `log_processor.h` and `log_processor.c`.

4. **Merge uses thread 0's tables as base** — instead of creating two fresh empty hash tables and merging all N threads into them, the merge step now takes thread 0's tables directly and merges threads 1..N-1 into them. Avoids allocating two extra tables and saves one merge pass.

## Key Concepts Demonstrated

1. **Thread creation and joining** — spawning parallel workers and waiting for completion
2. **Work partitioning** — dividing data into chunks for parallel processing
3. **Thread-local storage** — each thread has private results, avoiding race conditions
4. **Result merging** — combining partial results after parallel computation
5. **Hash tables in C** — manual implementation since C has no built-in dictionaries
6. **Performance measurement** — timing single vs multi-threaded execution
7. **Cross-platform threading** — pthreads (POSIX) vs WinAPI
