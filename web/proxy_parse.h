#ifndef PROXY_PARSE_H
#define PROXY_PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define CACHE_SIZE 10   // Max entries in LRU cache
#define BUFFER_SIZE 4096

// Cache Node structure
typedef struct CacheNode {
    char url[256];
    char response[BUFFER_SIZE];
    struct CacheNode* prev;
    struct CacheNode* next;
} CacheNode;

// LRU Cache
typedef struct {
    CacheNode* head;
    CacheNode* tail;
    int size;
    pthread_mutex_t lock;
} LRUCache;

// Function declarations
void init_cache(LRUCache* cache);
char* get_from_cache(LRUCache* cache, const char* url);
void put_in_cache(LRUCache* cache, const char* url, const char* response);
void free_cache(LRUCache* cache);

#endif