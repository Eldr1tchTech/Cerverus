#pragma once

#include "network_types.inl"

#include "core/containers/darray.h"

// Parses an URI into an provided darray. Also creates the darray.
darray* parse_URI(char* URI);

char *content_type_val_helper(const char *ext);

void darray_destroy_string_helper(darray* darr);