#include "string.h"
#include "core/memory/cmem.h"
#include <stddef.h>

#define default_lenth 8

typedef struct string_wrapper {
  size_t length;
  char data[];
} string_wrapper;

// TODO: There is probably a better way to do this.
string_wrapper *wrapper_from_string(string *str) {
  return (string_wrapper *)(str - sizeof(size_t));
}

string *wrapper_to_string(string_wrapper *wrapper) { return (string *)wrapper; }

string *str_new(size_t len) {
  string_wrapper *new_str =
      cmem_alloc(sizeof(string_wrapper) + sizeof(char) * (len == 0) ? 8 : len);
  new_str->data[0] = '\0';

  return wrapper_to_string(new_str);
}

string *str_from(char *str) {
  char *c = str;
  size_t len = 0;
  while (c) {
    *c++;
    len++;
  }

  if (len != 0) {
    return str_newlen(str, len);
  } else {
    return str_new(0);
  }
}

string *str_newlen(char *str, size_t len) {
  string_wrapper *new_str = wrapper_from_string(str_new(len));

  cmem_mcpy(new_str->data, void *src, size_t size)
  return wrapper_to_string(new_str);
}

void str_free(string *str);

size_t str_index_of(string *str, char c);

string *str_split(string *str, int i);

string *str_duplicate(string *str);
