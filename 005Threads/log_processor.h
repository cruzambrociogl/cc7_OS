#ifndef LOG_PROCESSOR_H
#define LOG_PROCESSOR_H

#define HASH_TABLE_SIZE 256
#define MAX_KEY_LEN 256
#define MAX_LINE_LEN 512

/* Hash table node (linked list for collisions) */
typedef struct HashNode {
    char key[MAX_KEY_LEN];
    int count;
    struct HashNode *next;
} HashNode;

/* Hash table */
typedef struct {
    HashNode *buckets[HASH_TABLE_SIZE];
} HashTable;

/* Per-thread local results (no synchronization needed) */
typedef struct {
    HashTable *ip_table;
    HashTable *url_table;
    int error_count;
} ThreadResult;

/* Chunk of lines assigned to a thread */
typedef struct {
    char **lines;
    int num_lines;
    ThreadResult result;
} ThreadTask;

/* Hash table operations */
HashTable *hash_table_create(void);
void hash_table_insert(HashTable *table, const char *key);
int hash_table_count_keys(HashTable *table);
void hash_table_merge(HashTable *dest, HashTable *src);
void hash_table_most_frequent(HashTable *table, char *out_key, int *out_count);
void hash_table_free(HashTable *table);

/* Log parsing */
int parse_log_line(const char *line, char *ip, char *url, int *status);

/* Thread worker function: processes a ThreadTask */
void process_chunk(ThreadTask *task);

/* File reading */
char **read_log_file(const char *filename, int *num_lines);
void free_lines(char **lines, int num_lines);

#endif
