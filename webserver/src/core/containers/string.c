#include "string.h"
#include "core/memory/cmem.h"

#define default_lenth 8

typedef struct string_header {
  size_t length;
  size_t capactiy;
  char data[];
} string_header;

string_header *string_to_header(string *str) {
  return (string_header *)(str - sizeof(string_header));
}

string header_to_string(string_header *wrapper) {
  return (string)wrapper->data;
}

string string_create(char *str) {
  size_t len = raw_string_length(str);
  string_header *header = cmem_alloc(sizeof(string_header) + len + 1);

  header->length = len;
  header->capactiy = len;
  cmem_mcpy(header->data, str, len + 1);

  return header_to_string(header);
}

size_t raw_string_length(const char *str) {
  if (str == NULL)
    return 0;

  size_t len = 0;
  while (*str != '\0') {
    len++;
    str++;
  }
  return len;
}