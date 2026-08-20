#include "string.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "darray.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

typedef struct string_header {
  size_t length;
  size_t capactiy;
  char data[];
} string_header;

// Utility
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

size_t string_get_u64_length(u64 n) {
  size_t length = 1;

  while (n >= 10) {
    n /= 10;
    length++;
  }

  return length;
}

size_t string_get_i32_length(i32 n) {
  size_t length = 1;

  while (n >= 10) {
    n /= 10;
    length++;
  }

  if (n < 0) {
    length++;
  }

  return length;
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

string string_empty() {
  string_header *header = cmem_alloc(sizeof(string_header) + 8 + 1);

  header->length = 0;
  header->capactiy = 8;
  cmem_zmem(header->data, 8);
  header->data[8] = '\0';

  return header_to_string(header);
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

bool string_parse_u64(string str, u64 *out) {
  size_t length = string_get_length(str);

  if (length == 0) {
    return false;
  }

  u64 value = 0;

  for (size_t i = 0; i < length; i++) {
    char c = str[i];

    if (c < '0' || c > '9') {
      return false;
    }

    u64 digit = (u64)(c - '0');

    if (value > (U64_MAX - digit) / 10) {
      return false;
    }

    value = value * 10 + digit;
  }

  *out = value;
  return true;
}

int _string_find(string str, const char *delim, size_t delim_len) {
  size_t length = string_get_length(str);

  if (delim_len > length) {
    return -1;
  }

  for (size_t i = 0; i <= length - delim_len; i++) {
    if (_raw_string_equal_length(&str[i], delim_len, delim, delim_len)) {
      return i;
    }
  }
  return -1;
}

void _string_set_length(string str, size_t new_len) {
  string_to_header(str)->length = new_len;
}

void string_remove_head(string str, size_t size) {
  size_t length = string_get_length(str);
  cmem_memmove(str, str + (size * sizeof(char)),
               length - size + 1); // +1 for null terminator
  _string_set_length(str, length - size);
}

string _string_split_size(string str, const char *delim, size_t delim_len) {
  int location = _string_find(str, delim, delim_len);
  if (location == -1) {
    return NULL;
  }

  location += delim_len;

  string token = _string_create_length(str, location);
  string_remove_head(str, location + delim_len);

  return token;
}

darray *_string_split_at_size(string str, const char *delim, size_t delim_len) {
  darray *string_darr = darray_create(8, sizeof(string *));
  string new_string;
  while ((new_string = _string_split_size(str, delim, delim_len)) != NULL) {
    darray_add(string_darr, &new_string);
  }

  if (string_darr->length != 0) {
    darray_add(string_darr, &str);
    return string_darr;
  } else {
    darray_destroy(string_darr);
    return NULL;
  }
}

bool string_parse_format(string str, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  const char *f = fmt;

  while (*f) {
    // %s
    if (f[0] == '%' && f[1] == 's') {
      string *out = va_arg(args, string *);
      f += 2;

      // Find the next %s in the format.
      const char *delim_start = f;

      while (*f && !(f[0] == '%' && f[1] == 's')) {
        f++;
      }

      size_t delim_len = f - delim_start;

      if (delim_len == 0) {
        // Final %s: consume the remainder of the input.
        *out = string_duplicate(str);
      } else {
        // %s is followed by a literal delimiter.
        *out = _string_split_size(str, delim_start, delim_len);

        if (*out == NULL) {
          LOG_DEBUG("string_parse_format - delimiter not found.");
          va_end(args);
          return false;
        }
      }

      continue;
    }

    // Literal character in the format.
    if (*str != *f) {
      LOG_DEBUG("string_parse_format - format violated.");
      va_end(args);
      return false;
    }

    str++;
    f++;
  }

  va_end(args);
  return true;
}

void _string_concatenate_string_size(char *str1, size_t *len1, char *str2,
                                     size_t len2) {
  cmem_mcpy(&str1[*len1], str2, len2);
  *len1 += len2;
  str1[*len1] = '\0';
}

bool string_concatenate_string(string str1, string str2) {
  if (string_get_capacity(str1) >
      string_get_length(str1) + string_get_length(str2)) {
    _string_concatenate_string_size(str1, &string_to_header(str1)->length, str2,
                                    string_get_length(str2));

    return true;
  }
  return false;
}