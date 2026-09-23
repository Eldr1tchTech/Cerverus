#include "util.h"

#include "core/containers/string.h"
#include "core/util/logger.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void darray_destroy_string_helper(darray darr) {
  string *darr_data = darr;
  for (size_t i = 0; i < *darray_get_length(darr_data); i++) {
    str_destroy(darr_data[i]);
  }
  darray_destroy(darr);
}

// rel is the path of the directory being walked, relative to root ("" for root
// itself, "/css" for root/css, ...).
static bool walk_dir(const char *root, const char *rel, int depth,
                     int max_depth, darray *out) {
  char dir_path[PATH_MAX];
  int n = snprintf(dir_path, sizeof dir_path, "%s%s", root, rel);
  if (n < 0 || (size_t)n >= sizeof dir_path) {
    LOG_WARNING("walk_dir - Path too long, skipping: %s%s", root, rel);
    return false;
  }

  DIR *dir = opendir(dir_path);
  if (!dir) {
    LOG_ERROR("walk_dir - Unable to open '%s': %s", dir_path, strerror(errno));
    return false;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    // Skips ".", ".." and hidden files (.git, .env, ...)
    if (entry->d_name[0] == '.')
      continue;

    unsigned char type = entry->d_type;
    if (type == DT_UNKNOWN) { // some filesystems don't fill d_type
      struct stat st;
      if (fstatat(dirfd(dir), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0)
        continue;
      type = S_ISDIR(st.st_mode)   ? DT_DIR
             : S_ISREG(st.st_mode) ? DT_REG
                                   : DT_UNKNOWN;
    }

    char entry_rel[PATH_MAX];
    n = snprintf(entry_rel, sizeof entry_rel, "%s/%s", rel, entry->d_name);
    if (n < 0 || (size_t)n >= sizeof entry_rel) {
      LOG_WARNING("walk_dir - Path too long, skipping: %s/%s", rel,
                  entry->d_name);
      continue;
    }

    if (type == DT_REG) {
      string path = str_create(entry_rel);
      *out = darray_add(*out, &path); // may reallocate, so always reassign
    } else if (type == DT_DIR) {
      if (max_depth < 0 || depth < max_depth) {
        walk_dir(root, entry_rel, depth + 1, max_depth, out);
      }
    }
    // symlinks and everything else are ignored
  }

  closedir(dir);
  return true;
}

darray fs_list_files(const char *root_dir, int max_depth) {
  darray files = darray_create(64, sizeof(string));

  if (!walk_dir(root_dir, "", 0, max_depth, &files)) {
    darray_destroy_string_helper(files);
    return nullptr;
  }

  return files;
}