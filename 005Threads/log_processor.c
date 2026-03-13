#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log_processor.h"

/* ---- Hash function ---- */
static unsigned int hash(const char *key) {
    unsigned int h = 0;
    while (*key) {
        h = h * 31 + (unsigned char)(*key);
        key++;
    }
    return h % HASH_TABLE_SIZE;
}

/* ---- Hash table operations ---- */

HashTable *hash_table_create(void) {
    HashTable *table = calloc(1, sizeof(HashTable));
    if (!table) {
        fprintf(stderr, "Failed to allocate hash table\n");
        exit(1);
    }
    return table;
}

static void hash_table_insert_count(HashTable *table, const char *key, int count) {
    unsigned int index = hash(key);
    HashNode *node = table->buckets[index];

    /* Search for existing key */
    while (node) {
        if (strcmp(node->key, key) == 0) {
            node->count += count;
            return;
        }
        node = node->next;
    }

    /* Key not found, create new node */
    HashNode *new_node = malloc(sizeof(HashNode));
    if (!new_node) {
        fprintf(stderr, "Failed to allocate hash node\n");
        exit(1);
    }
    strncpy(new_node->key, key, MAX_KEY_LEN - 1);
    new_node->key[MAX_KEY_LEN - 1] = '\0';
    new_node->count = count;
    new_node->next = table->buckets[index];
    table->buckets[index] = new_node;
}

void hash_table_insert(HashTable *table, const char *key) {
    hash_table_insert_count(table, key, 1);
}


int hash_table_count_keys(HashTable *table) {
    int count = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode *node = table->buckets[i];
        while (node) {
            count++;
            node = node->next;
        }
    }
    return count;
}

void hash_table_merge(HashTable *dest, HashTable *src) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode *node = src->buckets[i];
        while (node) {
            hash_table_insert_count(dest, node->key, node->count);
            node = node->next;
        }
    }
}

void hash_table_most_frequent(HashTable *table, char *out_key, int *out_count) {
    *out_count = 0;
    out_key[0] = '\0';
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode *node = table->buckets[i];
        while (node) {
            if (node->count > *out_count) {
                *out_count = node->count;
                strncpy(out_key, node->key, MAX_KEY_LEN - 1);
                out_key[MAX_KEY_LEN - 1] = '\0';
            }
            node = node->next;
        }
    }
}

void hash_table_free(HashTable *table) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode *node = table->buckets[i];
        while (node) {
            HashNode *temp = node;
            node = node->next;
            free(temp);
        }
    }
    free(table);
}

/* ---- Log line parsing ---- */
/* Format: IP - - [timestamp] "METHOD URL" STATUS */
int parse_log_line(const char *line, char *ip, char *url, int *status) {
    char method[16];

    /* Extract: IP - - [anything] "METHOD URL" STATUS */
    if (sscanf(line, "%255s %*s %*s %*s \"%15s %255[^\"]\" %d", ip, method, url, status) == 4) {
        return 1;
    }
    return 0;
}

/* ---- Thread worker ---- */
void process_chunk(ThreadTask *task) {
    task->result.ip_table = hash_table_create();
    task->result.url_table = hash_table_create();
    task->result.error_count = 0;

    char ip[MAX_KEY_LEN];
    char url[MAX_KEY_LEN];
    int status;

    for (int i = 0; i < task->num_lines; i++) {
        if (parse_log_line(task->lines[i], ip, url, &status)) {
            hash_table_insert(task->result.ip_table, ip);
            hash_table_insert(task->result.url_table, url);
            if (status >= 400 && status <= 599) {
                task->result.error_count++;
            }
        }
    }
}

/* ---- File reading ---- */
char **read_log_file(const char *filename, int *num_lines) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        exit(1);
    }

    int capacity = 1024;
    char **lines = malloc(capacity * sizeof(char *));
    *num_lines = 0;

    char buffer[MAX_LINE_LEN];
    while (fgets(buffer, MAX_LINE_LEN, file)) {
        /* Remove trailing newline */
        buffer[strcspn(buffer, "\n")] = '\0';

        if (*num_lines >= capacity) {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char *));
        }
        lines[*num_lines] = strdup(buffer);
        (*num_lines)++;
    }

    fclose(file);
    return lines;
}

void free_lines(char **lines, int num_lines) {
    for (int i = 0; i < num_lines; i++) {
        free(lines[i]);
    }
    free(lines);
}
