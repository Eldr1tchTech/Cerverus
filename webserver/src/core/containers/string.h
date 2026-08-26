#pragma once

#include "defines.h"

#include "core/containers/darray.h"

// NOTE: Internals functions are marked by beginning with an underscore (ex.
// _raw_string_equal_length)

// NOTE: This library is being built as need for certain functions arises. So
// not all classic utilities may be 'public'

/*
Dictionary:
string - str
string (without header) - cstr
length - len
concatenate - cat
format - fmt
duplicate - dup
*/

#define STR_LIT_LEN(lit) (sizeof(lit) - 1)

typedef const char *str_lit;
typedef char *cstr;
typedef char *string;

// Utility
size_t raw_str_len(const cstr str);

// Getters
size_t str_get_len(string str);
size_t str_get_capacity(string str);

// TODO: This naming is misleading
size_t str_get_u64_len(u64 n);

// Creating/Allocating?
string _str_create_len(const cstr str, size_t len);
string str_create(const cstr str);
string str_create_lit(str_lit lit);
string str_dup(string str);
string str_empty();
string str_grow_to(string str, size_t val);

void str_destroy(string str);

// Search
int _str_find(string str, str_lit delim, size_t delim_len);
#define _str_find_lit(str, lit) _str_find(str, lit, STR_LIT_LEN(lit))

// Comparisons
bool _raw_str_equal_len(const cstr str1, size_t len1, const cstr str2,
                        size_t len2);
bool str_equal(const string str1, const string str2);
#define str_equal_lit(s, lit)                                                  \
  _raw_str_equal_len((s), str_get_len(s), (lit), STR_LIT_LEN(lit))

// Parsing
bool str_parse_u64(string str, u64 *out);

/**
 * @brief Splits str at the first occurence of delim. str contains the contents
 * after the delim upon completion.
 *
 * @param str
 * @param delim If delim doesn't occur in str, nullptr is returned, str is not
 * considered consumed.
 * @param delim_len
 * @return string The contents before the delim
 */
string _str_split_size(string str, const cstr delim, size_t delim_len);
#define string_split_literal(str, delim)                                       \
  _str_split_size(str, delim, STR_LIT_LEN(delim))

/**
 * @brief Splits str at all occurences of delim. str is considered consumed upon
 * completion.
 *
 * @param str
 * @param delim If delim doesn't occur in str, nullptr is returned, str is not
 * considered consumed.
 * @param delim_len
 * @return darray*
 */
darray *_str_split_at_size(string str, const cstr delim, size_t delim_len);

#define str_split_at_lit(str, delim)                                           \
  _str_split_at_size(str, delim, STR_LIT_LEN(delim))

// TODO: add strict parsing at one point or another.
bool str_parse_fmt(string str, const cstr format, ...);

// Concatenating
void _str_cat_str_size(string str1, const cstr str2, size_t len2);

/**
 * @brief Concatenates str2 onto str1.
 *
 * @param str1
 * @param str2
 * @return bool If str1 capacity is overflowed, false is returned with no
 * changes made, otherwise true is returned.
 */
bool str_cat_str(string str1, string str2);
#define str_cat_str_lit(str1, lit)                                             \
  _str_cat_str_size(str1, lit, STR_LIT_LEN(lit))

void str_cat_u64(string str, u64 value);