#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "log_processor.h"

#define DEFAULT_THREADS 4

/* Thread function wrapper for WinAPI */
DWORD WINAPI thread_worker(LPVOID arg) {
    ThreadTask *task = (ThreadTask *)arg;
    process_chunk(task);
    return 0;
}

void run_analysis(char **lines, int num_lines, int num_threads) {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    HANDLE *threads = malloc(num_threads * sizeof(HANDLE));
    ThreadTask *tasks = malloc(num_threads * sizeof(ThreadTask));

    /* Divide lines among threads */
    int chunk_size = num_lines / num_threads;
    int remainder = num_lines % num_threads;
    int offset = 0;

    for (int i = 0; i < num_threads; i++) {
        tasks[i].lines = &lines[offset];
        tasks[i].num_lines = chunk_size + (i < remainder ? 1 : 0);
        offset += tasks[i].num_lines;

        threads[i] = CreateThread(NULL, 0, thread_worker, &tasks[i], 0, NULL);
        if (threads[i] == NULL) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            exit(1);
        }
    }

    /* Wait for all threads to finish */
    WaitForMultipleObjects(num_threads, threads, TRUE, INFINITE);

    QueryPerformanceCounter(&end);

    /* Close thread handles */
    for (int i = 0; i < num_threads; i++) {
        CloseHandle(threads[i]);
    }

    /* Merge results: use thread 0's tables as base */
    int total_errors = tasks[0].result.error_count;

    for (int i = 1; i < num_threads; i++) {
        hash_table_merge(tasks[0].result.ip_table, tasks[i].result.ip_table);
        hash_table_merge(tasks[0].result.url_table, tasks[i].result.url_table);
        total_errors += tasks[i].result.error_count;

        hash_table_free(tasks[i].result.ip_table);
        hash_table_free(tasks[i].result.url_table);
    }

    /* Print results */
    char most_visited[MAX_KEY_LEN];
    int most_count;
    hash_table_most_frequent(tasks[0].result.url_table, most_visited, &most_count);

    double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;

    printf("Threads: %d\n", num_threads);
    printf("Total Unique IPs: %d\n", hash_table_count_keys(tasks[0].result.ip_table));
    printf("Most Visited URL: %s (%d times)\n", most_visited, most_count);
    printf("HTTP Errors: %d\n", total_errors);
    printf("Execution Time: %.4f seconds\n", elapsed);

    /* Cleanup */
    hash_table_free(tasks[0].result.ip_table);
    hash_table_free(tasks[0].result.url_table);
    free(threads);
    free(tasks);
}

int main(int argc, char *argv[]) {
    const char *filename = "access.log";
    int num_threads = DEFAULT_THREADS;

    if (argc >= 2)
        filename = argv[1];
    if (argc >= 3)
        num_threads = atoi(argv[2]);

    if (num_threads < 1) {
        fprintf(stderr, "Number of threads must be at least 1\n");
        return 1;
    }

    printf("Reading log file: %s\n", filename);
    int num_lines;
    char **lines = read_log_file(filename, &num_lines);
    printf("Total lines: %d\n\n", num_lines);

    /* Single-threaded run */
    printf("--- Single-Threaded ---\n");
    run_analysis(lines, num_lines, 1);

    /* Multi-threaded run */
    if (num_threads > 1) {
        printf("\n--- Multi-Threaded ---\n");
        run_analysis(lines, num_lines, num_threads);
    }

    free_lines(lines, num_lines);
    return 0;
}
