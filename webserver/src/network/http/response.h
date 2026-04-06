#pragma once

#include "network_types.inl"

// Allocates the structure. Note this does not set ANY values.
response* response_create(int b_size);

void response_add_header(response* res, header h);

// Destroys the response and returns in serialized format.
char* response_serialize(response* res);