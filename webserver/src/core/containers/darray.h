#pragma once

// TODO: Change darray to use a header version, it's just cleaner and nicer
#include "defines.h"

typedef void *darray;

// Creation/Destruction
darray darray_create(size_t size, size_t stride);
void darray_destroy(darray darr);

// Getters/Setters
// TODO: rename to darray_size, length, etc.
size_t *darray_get_size(darray darr);
size_t *darray_get_length(darray darr);
size_t *darray_get_stride(darray darr);

// Accessors
darray darray_add(darray darr, void *element);

// Util
#define darray_foreach(darr, elem)                                             \
  for (typeof(darr) elem = (darr),                                             \
                    elem##_end = elem + *darray_get_length(darr);              \
       elem != elem##_end; elem++)