#pragma once

#include "defines.h"

#include <stdarg.h>
#include <stddef.h>

// Opaque handle - just a char*, plug-compatible with strlen/printf("%s")/
// strcmp/etc. Header metadata lives just behind the pointer, sds-style.
// Never free() this directly - use string_destroy.
typedef char *string;

// Creation / destruction
string string_create(const char *init); // NUL-terminated C string
string string_create_len(const void *init, size_t len); // may embed '\0'
string string_create_empty(void);
void string_destroy(string s);

// Introspection - O(1)
size_t string_len(const string s);
size_t string_capacity(const string s);
size_t string_avail(const string s);

// Ensures at least addlen bytes free beyond current length.
// Returns the (possibly relocated) string - always reassign the result.
string string_grow(string s, size_t addlen);

// Append
string string_cat(string s, const char *t);
string string_cat_len(string s, const void *t, size_t len);
string string_cat_string(string s, const string t);
string string_cat_format(string s, const char *fmt, ...);
string string_cat_vformat(string s, const char *fmt, va_list ap);

// Overwrite
string string_cpy(string s, const char *t);
string string_cpy_len(string s, const void *t, size_t len);

string string_dup(const string s);
void string_clear(string s); // keeps capacity, resets length to 0

int string_cmp(const string s1, const string s2);
