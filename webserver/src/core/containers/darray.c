#include "darray.h"

#include "core/memory/cmem.h"

darray *darray_create(int size, size_t stride) {
  darray *darr = cmem_alloc(sizeof(darray));
  darr->size = size;
  darr->length = 0;
  darr->stride = stride;

  darr->data = cmem_alloc(size * stride);
  cmem_zmem(darr->data, size * stride);

  return darr;
}

void darray_destroy(darray *darr) {
  cmem_free(darr->data);
  cmem_free(darr);
  darr = 0;
}

void darray_resize(darray *darr, int new_size) {
  void *temp = cmem_alloc(darr->stride * new_size);
  cmem_mcpy(temp, darr->data, darr->stride * darr->length);
  cmem_free(darr->data);
  darr->data = temp;
  darr->size = new_size;
}

void darray_add(darray *darr, void *element) {
  if (darr->length + 1 >= darr->size) {
    darray_resize(darr, darr->size * 2);
  }

  cmem_mcpy(darr->data + darr->stride * darr->length, element, darr->stride);
  darr->length++;
}

void *darray_get(darray *darr, int index) {
  return darr->data + (darr->stride * index);
}
