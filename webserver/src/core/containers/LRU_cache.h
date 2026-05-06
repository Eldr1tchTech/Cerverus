#pragma once

#include "defines.h"

#include "core/containers/doubly_linked_list.h"
#include "core/containers/hashmap.h"

// hashmap is present for fast lookups, no other reason.
typedef struct LRU_cache
{
    size_t size;
    size_t stride;
    doubly_linked_list* dll;
    hashmap* hmap;
} LRU_cache;

LRU_cache* LRU_cache_create(size_t size, size_t stride);
void LRU_cache_destroy(LRU_cache* cache);

void* LRU_cache_get();
void LRU_cache_add();
