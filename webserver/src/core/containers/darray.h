#pragma once

// TODO: Change darray to use a header version, it's just cleaner and nicer
#include "defines.h"

typedef struct darray {
  int size;
  int length;
  size_t stride;
  void *data;
} darray;

darray *darray_create(int size, size_t stride);
void darray_destroy(darray *darr);

void darray_add(darray *darr, void *element);

void *darray_get(darray *darr, int index);