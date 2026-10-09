#include "util.h"

#include "core/containers/darray.h"

#include "core/vendor/sds.h"
#include "defines.h"

#include <dirent.h>
#include <fcntl.h>

// LEFTOFF:
void darray_destroy_string_helper(darray darr) {
  sds *_darr = darr;
  darray_foreach(_darr, element) { sds_free(element); }
  darray_destroy(darr);
}

void recursive_walk(DIR *dir, const sds prefix, darray files_darr) {
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_name[0] == '.')
      continue;
    else if (entry->d_type == DT_REG) {
      sds temp_str = sdsnew(entry->d_name);
      sds new_entry = sdsdup(prefix);
      new_entry = sdscatsds(new_entry, temp_str);
      sds_free(temp_str);
      files_darr = darray_add(files_darr, &new_entry);
    } else if (entry->d_type == DT_DIR) {
      int fd = openat(dirfd(dir), entry->d_name, O_RDONLY | O_DIRECTORY);
      DIR *sub_dir = fdopendir(fd);

      sds sub_prefix = sdsdup(prefix);
      sds temp_str = sdsnew(entry->d_name);
      sub_prefix = sdscatsds(sub_prefix, temp_str);
      sds_free(temp_str);
      temp_str = sdsnewlit("/");
      sub_prefix = sdscatsds(sub_prefix, temp_str);
      sds_free(temp_str);

      struct dirent *sub_entry;
      recursive_walk(sub_dir, sub_prefix, files_darr);
      sds_free(sub_prefix);

      closedir(sub_dir);
    }
  }
}

darray _list_files(const cstr dir_path) {
  darray files_darr = darray_create(8, sizeof(sds));

  DIR *dir = opendir(dir_path);

  sds prefix = sdsnewlit("/");
  recursive_walk(dir, prefix, files_darr);
  sds_free(prefix);

  closedir(dir);

  return files_darr;
}