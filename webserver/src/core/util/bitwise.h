#pragma once

#include "defines.h"

typedef struct bitarray
{
    size_t size;
    char array[];
} bitarray;

bitarray* bitarray_create(size_t size);
void bitarray_destroy(bitarray* bit_arr);

void bitwise_set(bitarray* bit_arr, size_t index);
void bitwise_clear(bitarray* bit_arr, size_t index);
void bitwise_toggle(bitarray* bit_arr, size_t index);
bool bitwise_get(bitarray* bit_arr, size_t index);