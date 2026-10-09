#pragma once

#include "core/vendor/sds.h"

typedef unsigned long size_t;
typedef char *cstr;      // C string
typedef const char *lit; // string literal

#define sdsnewlit(lit) sdsnewlen("" lit, (sizeof(lit)) - 1)

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