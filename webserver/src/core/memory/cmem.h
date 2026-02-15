#pragma once

#include <stdlib.h>

typedef enum memory_tag {
    memory_tag_darray,
    memory_tag_router,
    memory_tag_server,
    memory_tag_request,
    memory_tag_response,
    memory_tag_string,
    memory_tag_queue,
    memory_tag_command_buffer,
} memory_tag;

void* cmem_alloc(memory_tag mem_tag, size_t size);

void cmem_free(memory_tag mem_tag, void* block);

void cmem_zmem(void* block, size_t size);

void cmem_mcpy(void* dest, void* source, size_t size);