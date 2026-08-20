#pragma once

#include "defines.h"

#include "core/containers/darray.h"

// NOTE: Internals functions are marked by beginning with an underscore (ex.
// _raw_string_equal_length)

// NOTE: This library is being built as need for certain functions arises. So
// not all classic utilities may be 'public'

#define STRING_LITERAL_LENGTH(lit) (sizeof(lit) - 1)

typedef char *string;

// Utility
size_t raw_string_length(const char *str);
size_t string_get_length(string str);
size_t string_get_capacity(string str);

size_t string_get_u64_length(u64 n);

// Creating
string _string_create_length(char *str, size_t length);
string string_create(char *str);
string string_duplicate(string str);
string string_empty();
void string_destroy(string str);

// Search
int _string_find(string str, const char *delim, size_t delim_len);
#define _string_find_literal(str, lit)                                         \
  _string_find(str, lit, STRING_LITERAL_LENGTH(lit))

// Comparisons
bool _raw_string_equal_length(const char *str1, size_t len1, const char *str2,
                              size_t len2);
bool string_equal(const string str1, const string str2);
#define string_equal_literal(s, lit)                                           \
  _raw_string_equal_length((s), string_get_length(s), (lit),                   \
                           STRING_LITERAL_LENGTH(lit))

// Parsing
bool string_parse_u64(string str, u64 *out);

/**
 * @brief Splits str at the first occurence of delim. str contains the contents
 * after the delim upon completion.
 *
 * @param str
 * @param delim If delim doesn't occur in str, NULL is returned, str is not
 * considered consumed.
 * @param delim_len
 * @return string The contents before the delim
 */
string _string_split_size(string str, const char *delim, size_t delim_len);
#define string_split_literal(str, delim)                                       \
  _string_split_size(str, delim, STRING_LITERAL_LENGTH(delim))

/**
 * @brief Splits str at all occurences of delim. str is considered consumed upon
 * completion.
 *
 * @param str
 * @param delim If delim doesn't occur in str, NULL is returned, str is not
 * considered consumed.
 * @param delim_len
 * @return darray*
 */
darray *_string_split_at_size(string str, const char *delim, size_t delim_len);

#define string_split_at_literal(str, delim)                                    \
  _string_split_at_size(str, delim, STRING_LITERAL_LENGTH(delim))

// TODO: add strict parsing at one point or another.
bool string_parse_format(string str, const char *format, ...);

// Concatenating

void _string_concatenate_string_size(char *str1, size_t *len1, char *str2,
                                     size_t len2);

/**
 * @brief Concatenates str2 onto str1.
 *
 * @param str1
 * @param str2
 * @return bool If str1 capacity is overflowed, false is returned with no
 * changes made, otherwise true is returned.
 */
bool string_concatenate_string(string str1, string str2);
#define string_concatenate_string_literal(str1, lit)                           \
  _string_concatenate_string_size(str1, &(str1 - (sizeof(size_t) * 2)), lit,   \
                                  STRING_LITERAL_LENGTH(lit))