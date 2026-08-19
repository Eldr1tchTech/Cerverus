#pragma once

#include "network/network_types.inl"

// Allocates the structure. Note this does not set ANY values.
response *response_create();

void response_add_header(response *res, header h);

// Destroys the response and returns in serialized format.
string response_serialize(response *res);