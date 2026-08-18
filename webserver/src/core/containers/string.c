#include "string.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include <stddef.h>

typedef struct string_header {
  size_t length;
  size_t capactiy;
  char data[];
} string_header;

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

// TODO: Make these just use negative array indexes? Need to implement some sort
// of asserts at one point or another
string_header *string_to_header(string str) {
  return (string_header *)(str - sizeof(string_header));
}

string header_to_string(string_header *wrapper) {
  return (string)wrapper->data;
}

size_t string_get_length(string str) { return string_to_header(str)->length; }

size_t string_get_capacity(string str) {
  return string_to_header(str)->capactiy;
}

// NOTE: When allocating do you want to alignof or pad the allocation/capacity
// so that its a multiple of 8 or so?
string _string_create_length(char *str, size_t length) {
  string_header *header = cmem_alloc(sizeof(string_header) + length + 1);

  header->length = length;
  header->capactiy = length;
  cmem_mcpy(header->data, str, length);
  header->data[length] = '\0';

  return header_to_string(header);
}

string string_create(char *str) {
  return _string_create_length(str, raw_string_length(str));
}

string string_duplicate(string str) {
  return _string_create_length(str, string_get_length(str));
}

void string_destroy(string str) { cmem_free(string_to_header(str)); }

bool _raw_string_equal_length(const char *str1, size_t len1, const char *str2,
                              size_t len2) {
  if (len1 == len2) {
    for (size_t i = 0; i < len1; i++) {
      if (str1[i] != str2[i]) {
        return false;
      }
    }
    return true;
  }

  return false;
}

bool string_equal(const string str1, const string str2) {
  return _raw_string_equal_length(str1, string_get_length(str1), str2,
                                  string_get_length(str2));
}

// returns the index of the first occurence of c, or -1 if not present
int _string_find(string str, const char c) {
  for (size_t i = 0; i < string_get_length(str); i++) {
    if (str[i] == c) {
      return i;
    }
  }
  return -1;
}

void _string_set_length(string str, size_t new_length) {
  string_to_header(str)->length = new_length;
}

void string_remove_head(string str, size_t size) {
  size_t length = string_get_length(str);
  cmem_memmove(str, str + (size * sizeof(char)),
               length - size + 1); // +1 for null terminator
  _string_set_length(str, length - size);
}

string string_split(string str, const char c) {
  int location = _string_find(str, c);
  if (location == -1) {
    return NULL;
  }
  string token = _string_create_length(str, location);
  string_remove_head(str, location + 1);

  return token;
}