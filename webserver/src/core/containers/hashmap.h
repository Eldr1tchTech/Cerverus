#pragma once

#include "core/containers/string.h"
#include "defines.h"

typedef size_t (*hash_fn)(string key);

typedef struct hashmap_entry {
  bool exists;
  string key;
  bool is_tombstone;
  char data[];
} hashmap_entry;

typedef struct hashmap {
  size_t size;
  size_t stride;
  hash_fn hash;
  hashmap_entry *entries;
} hashmap;

hashmap *hashmap_create(size_t size, double load, size_t stride, hash_fn hash);
void hashmap_destroy(hashmap *hmap);

/**
 * @brief Cleans up the hashmap, creating a cleaned up version to avoid
 * tombstone cluttering.
 *
 * @param hmap
 * @return hashmap*
 */
hashmap *hashmap_rehash(hashmap *hmap);

bool hashmap_set(hashmap *hmap, string key, void *element);
void *hashmap_get(hashmap *hmap, string key);
bool hashmap_delete(hashmap *hmap, string key);