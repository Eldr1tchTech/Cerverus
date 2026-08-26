#pragma once

#include "core/containers/darray.h"
#include "core/containers/string.h"

// Parses an URI into an provided darray. Also creates the darray.
darray *parse_URI(char *URI);

literal content_type_val_helper(string ext);

void darray_destroy_string_helper(darray *darr);