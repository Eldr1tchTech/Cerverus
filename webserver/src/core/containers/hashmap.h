#pragma once

#include "defines.h"

typedef struct hashmap_entry
{
    bool exists;
    char key[256];  // URI, e.g. "/index.html"
    char path[256]; // fs path, e.g. "assets/public/index.html"
} hashmap_entry;

typedef struct hashmap
{
    size_t capacity;
    hashmap_entry* entries;
} hashmap;

hashmap* hashmap_create();
void hashmap_destroy(hashmap* hmap);

void hashmap_set(hashmap* hmap, hashmap_entry entry);

bool hashmap_entry_exists(hashmap* hmap, char* key);

// Returns NULL if not found.
hashmap_entry* hashmap_get(hashmap* hmap, char* key);