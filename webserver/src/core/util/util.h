#pragma once

#include "core/containers/darray.h"

void darray_destroy_string_helper(darray darr);

/**
 * @brief Synchronously walks root_dir and collects the paths of all regular
 * files in it.
 *
 * @param root_dir Directory to walk (e.g. "assets/public").
 * @param max_depth How many directory levels below root_dir to descend into.
 * 0 = only files directly in root_dir, -1 = unlimited.
 * @return darray of `string`, each relative to root_dir with a leading '/'
 * (e.g. "/css/style.css"). nullptr if root_dir can't be opened. Free with
 * darray_destroy_string_helper.
 *
 * Hidden entries (names starting with '.') and symlinks are skipped.
 */
darray fs_list_files(const char *root_dir, int max_depth);