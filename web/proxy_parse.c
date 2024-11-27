#include "proxy_parse.h"

// Initialize the LRU Cache
void init_cache(LRUCache* cache) {
    cache->head = cache->tail = NULL;
    cache->size = 0;
    pthread_mutex_init(&cache->lock, NULL);
}

// Remove a node from the cache
void remove_node(LRUCache* cache, CacheNode* node) {
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    if (node == cache->head) cache->head = node->next;
    if (node == cache->tail) cache->tail = node->prev;

    free(node);
    cache->size--;
}

// Move a node to the head (most recently used)
void move_to_head(LRUCache* cache, CacheNode* node) {
    if (node == cache->head) return;

    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    if (node == cache->tail) cache->tail = node->prev;

    node->next = cache->head;
    node->prev = NULL;
    if (cache->head) cache->head->prev = node;
    cache->head = node;

    if (!cache->tail) cache->tail = node;
}

// Add a new entry to the cache
void put_in_cache(LRUCache* cache, const char* url, const char* response) {
    pthread_mutex_lock(&cache->lock);

    // Check if URL is already in cache
    CacheNode* current = cache->head;
    while (current) {
        if (strcmp(current->url, url) == 0) {
            strncpy(current->response, response, BUFFER_SIZE);
            move_to_head(cache, current);
            pthread_mutex_unlock(&cache->lock);
            return;
        }
        current = current->next;
    }

    // Create a new cache node
    CacheNode* new_node = (CacheNode*)malloc(sizeof(CacheNode));
    strncpy(new_node->url, url, 256);
    strncpy(new_node->response, response, BUFFER_SIZE);
    new_node->prev = NULL;
    new_node->next = NULL;

    // Add to the head of the list
    new_node->next = cache->head;
    if (cache->head) cache->head->prev = new_node;
    cache->head = new_node;

    if (!cache->tail) cache->tail = new_node;

    // Remove the least recently used node if cache exceeds size
    if (++cache->size > CACHE_SIZE) {
        remove_node(cache, cache->tail);
    }

    pthread_mutex_unlock(&cache->lock);
}

// Get a response from the cache
char* get_from_cache(LRUCache* cache, const char* url) {
    pthread_mutex_lock(&cache->lock);

    CacheNode* current = cache->head;
    while (current) {
        if (strcmp(current->url, url) == 0) {
            move_to_head(cache, current);
            pthread_mutex_unlock(&cache->lock);
            return current->response;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&cache->lock);
    return NULL; // Not found
}

// Free the entire cache
void free_cache(LRUCache* cache) {
    CacheNode* current = cache->head;
    while (current) {
        CacheNode* temp = current;
        current = current->next;
        free(temp);
    }
    pthread_mutex_destroy(&cache->lock);
}