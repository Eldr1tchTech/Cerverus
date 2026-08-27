#include "util.h"

#include "core/containers/string.h"

void darray_destroy_string_helper(darray darr) {
  string *darr_data = darr;
  for (size_t i = 0; i < *darray_get_length(darr_data); i++) {
    str_destroy(darr_data[i]);
  }
  darray_destroy(darr);
}