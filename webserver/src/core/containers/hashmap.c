#include "hashmap.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"

#include <string.h>

size_t hash_fnv1a(const char *key)
{
    size_t hash = 14695981039346656037ULL;
    while (*key)
    {
        hash ^= (unsigned char)*key++;
        hash *= 1099511628211ULL;
    }
    return hash;
}

hashmap_entry *hashmap_get_entry(hashmap *hmap, size_t index)
{
    return (hashmap_entry *)((char *)hmap->entries + ((sizeof(hashmap_entry) + hmap->stride) * index));
}


hashmap* hashmap_create(size_t size, double load, size_t stride, hash_fn hash) {
    hashmap *new_hmap = cmem_alloc(memory_tag_hashmap, sizeof(hashmap));

    if (load <= 0 || load > 1) LOG_DEBUG("hashmap_create - load should be between 0.0 and 1.0. load: %d", load);
    new_hmap->size = size / load;
    new_hmap->stride = stride;
    new_hmap->hash = hash ? hash : hash_fnv1a;
    new_hmap->entries = cmem_alloc(memory_tag_hashmap, (sizeof(hashmap_entry) + stride) * size);

    return new_hmap;
}

void hashmap_destroy(hashmap *hmap)
{
    for (size_t i = 0; i < hmap->size; i++)
    {
        hashmap_entry *curr_entry = hashmap_get_entry(hmap, i);
        if (curr_entry->exists)
        {
            cmem_free(memory_tag_hashmap, curr_entry->key);
        }
    }
    cmem_free(memory_tag_hashmap, hmap->entries);
    cmem_free(memory_tag_hashmap, hmap);
}

bool hashmap_set(hashmap *hmap, const char *key, void *element)
{
    size_t start = hmap->hash(key) % hmap->size;
    for (size_t i = 0; i < hmap->size; i++)
    {
        hashmap_entry *curr_entry = hashmap_get_entry(hmap, (start + i) % hmap->size);
        if (curr_entry->exists || curr_entry->is_tombstone)
        {
            if (curr_entry->exists && strcmp(curr_entry->key, key) == 0)
            {
                cmem_mcpy(curr_entry->data, element, hmap->stride);
                if (curr_entry->key)
                    cmem_free(memory_tag_hashmap, curr_entry->key);
                curr_entry->key = cmem_alloc(memory_tag_hashmap, strlen(key) + 1);
                strcpy(curr_entry->key, key);
                curr_entry->exists = true;
                return true;
            }
            continue;
        }
        cmem_mcpy(curr_entry->data, element, hmap->stride);
        if (curr_entry->key)
            cmem_free(memory_tag_hashmap, curr_entry->key);
        curr_entry->key = cmem_alloc(memory_tag_hashmap, strlen(key) + 1);
        strcpy(curr_entry->key, key);
        curr_entry->exists = true;
        return true;
    }
    return false;
}

void *hashmap_get(hashmap *hmap, const char *key)
{
    size_t start = hmap->hash(key) % hmap->size;
    for (size_t i = 0; i < hmap->size; i++)
    {
        hashmap_entry *curr_entry = hashmap_get_entry(hmap, (start + i) % hmap->size);
        if (curr_entry->exists || curr_entry->is_tombstone)
        {
            if (curr_entry->exists && strcmp(curr_entry->key, key) == 0)
            {
                return curr_entry->data;
            }
            continue;
        }
        return NULL;
    }
    return NULL;
}

hashmap* hashmap_rehash(hashmap* hmap) {
    hashmap* new_hmap = hashmap_create(hmap->size, 1, hmap->stride, hmap->hash);

    hashmap_entry* curr_entry;
    for (size_t i = 0; i < hmap->size; i++)
    {
        curr_entry = hashmap_get_entry(hmap, i);
        if (curr_entry->exists)
        {
            hashmap_set(new_hmap, curr_entry->key, curr_entry->data);
        }
    }

    hashmap_destroy(hmap);
    
    return new_hmap;
}

bool hashmap_delete(hashmap* hmap, const char* key) {
    size_t start = hmap->hash(key) % hmap->size;
    for (size_t i = 0; i < hmap->size; i++)
    {
        hashmap_entry *curr_entry = hashmap_get_entry(hmap, (start + i) % hmap->size);
        if (curr_entry->exists || curr_entry->is_tombstone)
        {
            if (curr_entry->exists && strcmp(curr_entry->key, key) == 0)
            {
                cmem_zmem(curr_entry, sizeof(hashmap_entry) + hmap->stride);
                curr_entry->is_tombstone = true;
                return true;
            }
            continue;
        }
        return false;
    }
    return false;
}