#pragma once

#include "network_types.inl"

// Allocates the structure. Note this does not set ANY values.
response* response_create(int h_count, int b_size);

// Destroys the response and returns in serialized format.
char* response_serialize(response* res);

char* content_type_val_helper(const char* ext);