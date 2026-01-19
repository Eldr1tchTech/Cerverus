#pragma once

// Figure out some replacement for size_t
#include "defines.h"

typedef struct darray
{
    int size;
    int length;
    size_t stride;
    void* data;
} darray;

darray* darray_create(int size, size_t stride);
void darray_destroy(darray* darr);

void darray_add(darray* darr, void* element);

// TODO: darray_pop_at, etc. More utility functions.