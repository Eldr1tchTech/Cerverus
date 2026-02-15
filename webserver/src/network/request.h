#pragma once

#include "network_types.inl"

// Creates a request struct with the given information. Handles allocation.
request* request_parse(char* raw_req);

// Frees passed request and destroys pointer.
void request_destroy(request* req);