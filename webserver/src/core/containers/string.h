#pragma once

#include "defines.h"

// NOTE: Internals functions are marked by beginning with an underscore (ex.
// _raw_string_equal_length)
// TODO: Move internals functions here (into the header file).

#define STRING_LITERAL_LENGTH(lit) (sizeof(lit) - 1)

typedef char *string;

// Utility
size_t raw_string_length(const char *str);
size_t string_get_length(string str);
size_t string_get_capacity(string str);

// Creating
string string_create(char *str);
string string_duplicate(string str);
void string_destroy(string str);

// Comparisons
bool _raw_string_equal_length(const char *str1, size_t len1, const char *str2,
                              size_t len2);
bool string_equal(const string str1, const string str2);
#define string_equal_literal(s, lit)                                           \
  _raw_string_equal_length((s), string_get_length(s), (lit),                   \
                           STRING_LITERAL_LENGTH(lit))

// Splits at the first appearance of character c. Returns a new string up to c.
// Original contains everything following c. If not found, NULL is returned and
// no modification is made to the original string.
string string_split(string str, const char c);