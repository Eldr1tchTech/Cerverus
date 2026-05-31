#include "bitwise.h"

#include "core/memory/cmem.h"

bitarray* bitarray_create(size_t size) {
    bitarray* new_bit_arr = cmem_alloc(memory_tag_unknown, sizeof(bitarray) + (size + 7) / 8);
    new_bit_arr->size = size;

    return new_bit_arr;
}

void bitarray_destroy(bitarray* bit_arr) {
    cmem_free(memory_tag_unknown, bit_arr);
}

void bitwise_set(bitarray* bit_arr, size_t index) {
    bit_arr->array[index / 8] |= (1 << (index % 8));
}

void bitwise_clear(bitarray* bit_arr, size_t index) {
    bit_arr->array[index / 8] &= ~(1 << (index % 8));
}

void bitwise_toggle(bitarray* bit_arr, size_t index) {
    bit_arr->array[index / 8] ^= (1 << (index % 8));
}

bool bitwise_get(bitarray* bit_arr, size_t index) {
    return (bit_arr->array[index / 8] >> (index % 8)) & 1;
}