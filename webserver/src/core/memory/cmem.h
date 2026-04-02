#pragma once

#include <stdlib.h>

#define MEMORY_TAGS(X) \
    X(memory_tag_darray, "[DARRAY]") \
    X(memory_tag_server, "[SERVER]") \
    X(memory_tag_request, "[REQUEST]") \
    X(memory_tag_response, "[RESPONSE]") \
    X(memory_tag_string, "[STRING]") \
    X(memory_tag_queue, "[QUEUE]") \
    X(memory_tag_trie, "[TRIE]") \
    X(memory_tag_route, "[ROUTE]") \
    X(memory_tag_io_uring, "[IO_URING]")

#define X_ENUM(tag, name) tag,
typedef enum memory_tag {
    MEMORY_TAGS(X_ENUM)
    memory_tag_max_tags,
} memory_tag;
#undef GENERATE_ENUM

typedef struct cmem_state
{
    int allocations[memory_tag_max_tags];
    int allocated[memory_tag_max_tags];
} cmem_state;

void* cmem_alloc(memory_tag mem_tag, size_t size);

void cmem_free(memory_tag mem_tag, void* block);

void cmem_zmem(void* block, size_t size);

void cmem_mcpy(void* dest, void* src, size_t size);

void cmem_print_stats();