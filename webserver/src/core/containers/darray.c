#include "darray.h"

#include "core/memory/cmem.h"
#include <stddef.h>

typedef struct darray_header {
  size_t size;
  size_t length;
  size_t stride;
  char data[];
} darray_header;

darray_header *header_from_darray(darray darr) {
  return (darray_header *)darr - sizeof(darray_header);
}

darray darray_create(size_t size, size_t stride) {
  darray_header *darr_h = cmem_alloc(sizeof(darray_header) + stride * size);
  darr_h->size = size;
  darr_h->length = 0;
  darr_h->stride = stride;

  return darr_h->data;
}

void darray_destroy(darray darr) { cmem_free(darr); }

darray darray_resize(darray darr, size_t new_size) {
  darray temp_darr = darray_create(new_size, *darray_get_stride(darr));
  cmem_mcpy(temp_darr, darr,
            *darray_get_stride(darr) * *darray_get_length(darr));
  return temp_darr;
}

// Getters/Setters
size_t *darray_get_size(darray darr) {
  darray_header *darr_h = header_from_darray(darr);
  return &darr_h->size;
}
size_t *darray_get_length(darray darr) {
  darray_header *darr_h = header_from_darray(darr);
  return &darr_h->length;
}
size_t *darray_get_stride(darray darr) {
  darray_header *darr_h = header_from_darray(darr);
  return &darr_h->stride;
}

darray darray_add(darray darr, void *element) {
  if (*darray_get_length(darr) + 1 >= *darray_get_size(darr)) {
    darr = darray_resize(darr, *darray_get_size(darr) * 2);
  }

  cmem_mcpy(darr + *darray_get_stride(darr) * *darray_get_length(darr), element,
            *darray_get_stride(darr));
  *darray_get_length(darr) += 1;
  return darr;
}