#pragma once

typedef unsigned long size_t;

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS
#elif defined(__linux__)
#define PLATFORM_LINUX
#else
#define PLATFORM_UNKNOWN
#endif