#pragma once

#include "defines.h"

#include "core/containers/doubly_linked_list.h"
#include "core/containers/hashmap.h"

typedef void (*eviction_handler)(void* item);

typedef struct LRU_cache
{
    size_t size;
    size_t stride;
    doubly_linked_list* dll;
    hashmap* hmap;
    eviction_handler evic_handler;
} LRU_cache;

LRU_cache* LRU_cache_create(size_t size, size_t stride, eviction_handler evic_handler);
void LRU_cache_destroy(LRU_cache* cache);

void* LRU_cache_get(LRU_cache* cache, char* label);
void LRU_cache_add(LRU_cache* cache, char* label, void* item);
void LRU_cache_rehash(LRU_cache* cache);