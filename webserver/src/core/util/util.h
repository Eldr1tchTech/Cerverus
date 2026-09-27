#pragma once

#include "core/containers/darray.h"
#include "core/containers/string.h"

void darray_destroy_string_helper(darray darr);

darray _list_files(const cstr root_path);