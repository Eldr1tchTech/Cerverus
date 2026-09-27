#include "util.h"

#include "core/containers/darray.h"
#include "core/containers/string.h"
#include "core/util/logger.h"

#include <dirent.h>
#include <fcntl.h>

void darray_destroy_string_helper(darray darr) {
  string *darr_data = darr;
  for (size_t i = 0; i < *darray_get_length(darr_data); i++) {
    str_destroy(darr_data[i]);
  }
  darray_destroy(darr);
}

void recursive_walk(DIR *dir, const string prefix, darray files_darr) {
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_name[0] == '.')
      continue;
    else if (entry->d_type == DT_REG) {
      string temp_str = str_create(entry->d_name);
      string new_entry = str_dup(prefix);
      new_entry =
          str_cat_str(new_entry, temp_str); // Will I need to rework this as it
                                            // could have to reallocate?
      str_destroy(temp_str);
      files_darr = darray_add(files_darr, &new_entry);
    } else if (entry->d_type == DT_DIR) {
      int fd = openat(dirfd(dir), entry->d_name, O_RDONLY | O_DIRECTORY);
      DIR *sub_dir = fdopendir(fd);

      string sub_prefix = str_dup(prefix);
      string temp_str = str_create(entry->d_name);
      sub_prefix = str_cat_str(sub_prefix, temp_str);
      str_destroy(temp_str);
      temp_str = str_create_lit("/");
      sub_prefix = str_cat_str(sub_prefix, temp_str);
      str_destroy(temp_str);

      struct dirent *sub_entry;
      recursive_walk(sub_dir, sub_prefix, files_darr);
      str_destroy(sub_prefix);

      closedir(sub_dir);
    }
  }
}

darray _list_files(const cstr dir_path) {
  darray files_darr = darray_create(8, sizeof(string));

  DIR *dir = opendir(dir_path);

  string prefix = str_create_lit("/");
  recursive_walk(dir, prefix, files_darr);
  str_destroy(prefix);

  closedir(dir);

  return files_darr;
}