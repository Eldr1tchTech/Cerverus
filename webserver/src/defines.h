#pragma once

#include "core/vendor/sds.h"
#include <string.h>

typedef unsigned long size_t;
typedef char *cstr_t;      // C string
typedef const char *lit_t; // string literal

#define sdsnewlit(lit) sdsnewlen("" lit, (sizeof(lit)) - 1)

static inline int sdscmplitlen(const sds s, lit_t lit, size_t litlen) {
  size_t len = sdslen(s);
  size_t minlen = len < litlen ? len : litlen;
  int cmp = memcmp(s, lit, minlen);
  if (cmp == 0)
    return len > litlen ? 1 : (len < litlen ? -1 : 0);
  return cmp;
}

#define sdscmplit(s, lit) sdscmplitlen((s), "" lit, sizeof(lit) - 1)

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