#include "hashmap.h"

#include "core/containers/darray.h"
#include "core/memory/cmem.h"
#include "core/util/util.h"
#include "core/util/logger.h"

#include <dirent.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define BASE_PATH "assets/public/"

static void map_directory(const char *path, size_t root_len, darray *darr)
{
    DIR *dir = opendir(path);
    if (!dir)
    {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (entry->d_type == DT_REG)
        {
            const char *relative = path + root_len;
            char *entry_path = asprintf_cerv("/%s%s", relative, entry->d_name);
            darray_add(darr, &entry_path);
        }
        else if (entry->d_type == DT_DIR)
        {
            char *sub_path = asprintf_cerv("%s%s/", path, entry->d_name);
            map_directory(sub_path, root_len, darr);
            cmem_free(memory_tag_string, sub_path);
        }
    }

    closedir(dir);
}

static size_t next_pow2(size_t a)
{
    size_t b = 2;
    while (b < a)
        b <<= 1;   // was b ^= 2, which is wrong
    return b;
}

static uint32_t fnv1a(const char *key)
{
    uint32_t h = 2166136261u;
    while (*key)
    {
        h ^= (unsigned char)*key++;
        h *= 16777619u;
    }
    return h;
}

hashmap *hashmap_create()
{
    hashmap *hmap = cmem_alloc(memory_tag_hashmap, sizeof(hashmap));

    darray *dir_darr = darray_create(8, sizeof(char *));
    map_directory(BASE_PATH, strlen(BASE_PATH), dir_darr);

    hmap->capacity = next_pow2((size_t)(dir_darr->length / 0.65f));
    hmap->entries = cmem_alloc(memory_tag_hashmap, hmap->capacity * sizeof(hashmap_entry));

    char **darr_data = dir_darr->data;
    for (size_t i = 0; i < (size_t)dir_darr->length; i++)
    {
        hashmap_entry e = {0};
        e.exists = true;

        // key: URI, e.g. "/index.html"
        strncpy(e.key, darr_data[i], sizeof(e.key) - 1);

        // path: fs path, e.g. "assets/public/index.html"
        snprintf(e.path, sizeof(e.path), BASE_PATH "%s", darr_data[i] + 1); // +1 skips leading /

        hashmap_set(hmap, e);

        cmem_free(memory_tag_string, darr_data[i]);
    }

    LOG_INFO("hashmap_create - Indexed %i assets.", dir_darr->length);

    darray_destroy(dir_darr);  // frees the darray itself; strings already freed above

    return hmap;
}

void hashmap_destroy(hashmap *hmap)
{
    cmem_free(memory_tag_hashmap, hmap->entries);
    cmem_free(memory_tag_hashmap, hmap);
}

void hashmap_set(hashmap *hmap, hashmap_entry entry)
{
    uint32_t slot = fnv1a(entry.key) & (hmap->capacity - 1);

    while (hmap->entries[slot].exists)
    {
        // Overwrite if the key already exists (idempotent re-index)
        if (strncmp(hmap->entries[slot].key, entry.key, sizeof(entry.key)) == 0)
            break;

        slot = (slot + 1) & (hmap->capacity - 1);
    }

    hmap->entries[slot] = entry;
}

hashmap_entry *hashmap_get(hashmap *hmap, char *key)
{
    uint32_t slot = fnv1a(key) & (hmap->capacity - 1);

    while (hmap->entries[slot].exists)
    {
        if (strncmp(hmap->entries[slot].key, key, sizeof(hmap->entries[slot].key)) == 0)
            return &hmap->entries[slot];

        slot = (slot + 1) & (hmap->capacity - 1);
    }

    return NULL;
}

bool hashmap_entry_exists(hashmap *hmap, char *key)
{
    return hashmap_get(hmap, key) != NULL;
}