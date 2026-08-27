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
size_t raw_str_len(const cstr str) {
  if (str == nullptr)
    return 0;

  cstr temp_str = str;
  size_t len = 0;
  while (*str != '\0') {
    len++;
    temp_str++;
  }
  return len;
}

// TODO: Make these just use negative array indexes? Need to implement some sort
// of asserts at one point or another
string_header *string_get_header(string str) {
  return (string_header *)(str - sizeof(string_header));
}

string header_to_string(string_header *wrapper) {
  return (string)wrapper->data;
}

size_t str_get_len(string str) { return string_get_header(str)->length; }

size_t str_get_capacity(string str) { return string_get_header(str)->capactiy; }

size_t str_get_u64_len(u64 n) {
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

string _str_create_len(char *str, size_t length) {
  string_header *header = cmem_alloc(sizeof(string_header) + length + 1);

  header->length = length;
  header->capactiy = length;
  cmem_mcpy(header->data, str, length);
  header->data[length] = '\0';

  return header_to_string(header);
}

string str_create(char *str) { return _str_create_len(str, raw_str_len(str)); }

string str_create_lit(str_lit lit) {
  return _str_create_len(lit, STR_LIT_LEN(lit));
}

string str_dup(string str) { return _str_create_len(str, str_get_len(str)); }

string str_empty() {
  string_header *header = cmem_alloc(sizeof(string_header) + 8 + 1);

  header->length = 0;
  header->capactiy = 8;
  cmem_zmem(header->data, 8);
  header->data[8] = '\0';

  return header_to_string(header);
}

string str_grow_to(string str, size_t value) {
  string_header *header = string_get_header(str);

  if (header->capactiy >= value) {
    return str; // already big enough, no-op
  }

  string_header *new_header = cmem_alloc(sizeof(string_header) + value + 1);

  new_header->length = header->length;
  new_header->capactiy = value;
  cmem_mcpy(new_header->data, header->data,
            header->length + 1); // includes '\0'

  str_destroy(str);

  return header_to_string(new_header);
}

void str_destroy(string str) { cmem_free(string_get_header(str)); }

bool _raw_str_equal_len(const cstr str1, size_t len1, const cstr str2,
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

bool str_equal(const string str1, const string str2) {
  return _raw_str_equal_len(str1, str_get_len(str1), str2, str_get_len(str2));
}

bool str_parse_u64(string str, u64 *out) {
  size_t length = str_get_len(str);

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

int _str_find(string str, const char *delim, size_t delim_len) {
  size_t length = str_get_len(str);

  if (delim_len > length) {
    return -1;
  }

  for (size_t i = 0; i <= length - delim_len; i++) {
    if (_raw_str_equal_len(&str[i], delim_len, delim, delim_len)) {
      return i;
    }
  }
  return -1;
}

void _string_set_length(string str, size_t new_len) {
  string_get_header(str)->length = new_len;
}

void string_remove_head(string str, size_t size) {
  size_t length = str_get_len(str);
  cmem_memmove(str, str + (size * sizeof(char)),
               length - size + 1); // +1 for null terminator
  _string_set_length(str, length - size);
}

string _str_split_size(string str, const cstr delim, size_t delim_len) {
  int location = _str_find(str, delim, delim_len);
  if (location == -1) {
    return nullptr;
  }

  location += delim_len;

  string token = _str_create_len(str, location);
  string_remove_head(str, location + delim_len);

  return token;
}

darray _str_split_at_size(string str, const cstr delim, size_t delim_len) {
  darray string_darr = darray_create(8, sizeof(string *));
  string new_string;
  while ((new_string = _str_split_size(str, delim, delim_len)) != nullptr) {
    darray_add(string_darr, &new_string);
  }

  if (darray_get_length(string_darr) != 0) {
    darray_add(string_darr, &str);
    return string_darr;
  } else {
    darray_destroy(string_darr);
    return nullptr;
  }
}

bool str_parse_fmt(string str, const cstr fmt, ...) {
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
        *out = str_dup(str);
      } else {
        // %s is followed by a literal delimiter.
        *out = _str_split_size(str, delim_start, delim_len);

        if (*out == nullptr) {
          LOG_DEBUG("str_parse_fmt - delimiter not found.");
          va_end(args);
          return false;
        }
      }

      continue;
    }

    // Literal character in the format.
    if (*str != *f) {
      LOG_DEBUG("str_parse_fmt - format violated.");
      va_end(args);
      return false;
    }

    str++;
    f++;
  }

  va_end(args);
  return true;
}

void _str_cat_str_size(char *str1, char *str2, size_t len2) {
  size_t *len1 = &string_get_header(str1)->length;
  cmem_mcpy(&str1[*len1], str2, len2);
  *len1 += len2;
  str1[*len1] = '\0';
}

bool str_cat_str(string str1, string str2) {
  if (str_get_capacity(str1) > str_get_len(str1) + str_get_len(str2)) {
    _str_cat_str_size(str1, str2, str_get_len(str2));

    return true;
  }
  return false;
}

// WARN: VIBED
void str_cat_u64(string str, u64 value) {
  string_header *header = string_get_header(str);

  char digits[20]; // max digits for u64 (18446744073709551615)
  size_t n = 0;

  if (value == 0) {
    digits[n++] = '0';
  } else {
    while (value > 0) {
      digits[n++] = '0' + (value % 10);
      value /= 10;
    }
  }

  size_t pos = header->length;
  for (size_t i = 0; i < n; i++) {
    str[pos + i] = digits[n - 1 - i]; // reverse while copying out
  }

  pos += n;
  str[pos] = '\0';
  header->length = pos;
}