#pragma once

#include "core/memory/cmem.h"
#include "core/util/bitwise.h"

typedef struct pool
{
    size_t stride;
    bitarray* bit_arr;
    char data[];
} pool;
