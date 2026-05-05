#include "hashmap.h"

#include <core/memory/cmem.h>

#include <string.h>

size_t hash_fn_fnv1a(const char *key) {
    size_t hash = 14695981039346656037ULL;
    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= 1099511628211ULL;
    }
    return hash;
}

hashmap* hashmap_create(size_t size, size_t stride) {
    hashmap* hmap = cmem_alloc(memory_tag_hashmap, sizeof(hashmap));

    hmap->size = size;
    hmap->stride = stride;
    hmap->hash = hash_fn_fnv1a;
    hmap->entries = cmem_alloc(memory_tag_hashmap, (sizeof(hashmap_entry) + stride) * size);

    return hmap;
}

void hashmap_destroy(hashmap* hmap) {
    cmem_free(memory_tag_hashmap, hmap->entries);
    cmem_free(memory_tag_hashmap, hmap);
}

bool hashmap_set(hashmap* hmap, const char* key, void* element) {
    for (size_t i = hmap->hash(key) % hmap->size; i < hmap->size; i++)
    {
        hashmap_entry* curr_entry = &hmap->entries[i];
        if (curr_entry->exists)
        {
            if (strcmp(curr_entry->key, key) == 0)
            {
                cmem_mcpy(curr_entry->data, element, hmap->stride);
                curr_entry->key = cmem_alloc(memory_tag_hashmap, strlen(key) + 1);
                strcpy(curr_entry->key, key);
                curr_entry->exists = true;
                return true;
            }
            continue;
        }
        cmem_mcpy(curr_entry->data, element, hmap->stride);
        curr_entry->key = cmem_alloc(memory_tag_hashmap, strlen(key) + 1);
        strcpy(curr_entry->key, key);
        curr_entry->exists = true;
        return true;
    }
    return false;
}

void* hashmap_get(hashmap* hmap, const char* key) {
    for (size_t i = hmap->hash(key) % hmap->size; i < hmap->size; i++)
    {
        hashmap_entry* curr_entry = &hmap->entries[i];
        if (curr_entry->exists)
        {
            if (strcmp(curr_entry, key) == 0)
            {
                return curr_entry->data;
            }
            continue;
        }
        return NULL;
    }
}