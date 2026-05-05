#pragma once

#include <stddef.h>

typedef size_t (*hash_fn)(const char* key);

typedef struct hashmap_entry
{
    bool exists;
    const char* key;
    char data[];
} hashmap_entry;

typedef struct hashmap
{
    size_t size;
    size_t stride;
    hash_fn hash;
    hashmap_entry* entries;
} hashmap;

hashmap* hashmap_create(size_t size, size_t stride);
void hashmap_destroy(hashmap* hmap);

bool hashmap_set(hashmap* hmap, const char* key, void* element);
void* hashmap_get(hashmap* hmap, const char* key);