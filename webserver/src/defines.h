#pragma once

#include "core/vendor/sds.h"
#include <string.h>

typedef unsigned long size_t;
typedef char *cstr_t;      // C string
typedef const char *lit_t; // string literal

#define litlen(lit) sizeof("" lit) - 1

#define sdsnewlit(lit) sdsnewlen("" lit, litlen(lit))

static inline int _strcmp(const cstr_t str1, const size_t len1, const cstr_t str2, const size_t len2) {
  size_t minlen = len1 < len2 ? len1 : len2;
  int cmp = memcmp(str1, str2, minlen);
  if (cmp == 0)
    return len1 > len2 ? 1 : (len1 < len2 ? -1 : 0);
  return cmp;
}

static inline int sdscmplitlen(const sds s, lit_t lit, size_t litlen) {
  return _strcmp(s, sdslen(s), lit, litlen);
}

#define sdscmplit(s, lit) sdscmplitlen((s), "" lit, litlen(lit))

#define sdseqlit(s, lit) (sdscmplit((s), lit) == 0)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed int i32;

#define U64_MAX ((u64) - 1)

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS
#elif defined(__linux__)
#define PLATFORM_LINUX
#else
#define PLATFORM_UNKNOWN
#endif