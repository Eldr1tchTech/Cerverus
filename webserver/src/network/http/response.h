#pragma once

#include "network/network_types.inl"

// Allocates the structure. Note this does not set ANY values.
response_t *response_create();

void response_add_header(response_t *res, header_t h);

// Destroys the response and returns in serialized format.
sds response_serialize(response_t *res);