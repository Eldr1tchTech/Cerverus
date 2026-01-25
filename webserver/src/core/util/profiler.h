#include "core/util/logger.h"

#include <time.h>
#include <sys/time.h>

#define profile_operation(name, operation) do { \
    struct timespec start; \
    clock_gettime(CLOCK_MONOTONIC, &start); \
    operation; \
    struct timespec end; \
    clock_gettime(CLOCK_MONOTONIC, &end); \
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0; \
    LOG_INFO("Operation %s took %.6f seconds to run.", name, elapsed); \
} while(0)