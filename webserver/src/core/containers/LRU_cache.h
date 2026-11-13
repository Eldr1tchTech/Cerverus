#pragma once

#include "defines.h"

#include "core/containers/doubly_linked_list.h"
#include "core/containers/hashmap.h"

typedef void (*eviction_handler_fn)(void *item);

typedef struct LRU_cache_t {
  size_t size;
  size_t stride;
  doubly_linked_list *dll;
  hashmap *hmap;
  eviction_handler_fn evic_handler;
} LRU_cache_t;

LRU_cache_t *LRU_cache_create(size_t size, size_t stride,
                            eviction_handler_fn evic_handler);
void LRU_cache_destroy(LRU_cache_t *cache);

void *LRU_cache_get(LRU_cache_t *cache, sv_t label);
void LRU_cache_add(LRU_cache_t *cache, sv_t label, void *item);
