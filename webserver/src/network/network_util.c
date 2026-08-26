#include "network_util.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"

darray *parse_URI(string URI) { return string_split_at_literal(URI, "/"); }

string content_type_val_helper(string ext) {
  if (ext) {

    if (string_equal_literal(ext, "html")) {
      return "text/html";
    } else if (string_equal_literal(ext, "css")) {
      return "text/css";
    } else if (string_equal_literal(ext, "jpg") ||
               string_equal_literal(ext, "jpeg")) {
      return "image/jpeg";
    } else if (string_equal_literal(ext, "png")) {
      return "image/png";
    } else if (string_equal_literal(ext, "gif")) {
      return "image/gif";
    } else if (string_equal_literal(ext, "webp")) {
      return "image/webp";
    } else if (string_equal_literal(ext, "svg")) {
      return "image/svg+xml";
    } else if (string_equal_literal(ext, "ico")) {
      return "image/x-icon";
    } else {
      LOG_ERROR("content_type_val_helper - Currently unsuported file "
                "extension: %s. Returning null.",
                ext);
      return nullptr;
    }
  }
  LOG_ERROR("content_type_val_helper - Please provide a valid char* for ext.");
  return nullptr;
}

void darray_destroy_string_helper(darray *darr) {
  char **darr_data = darr->data;
  for (size_t i = 0; i < darr->length; i++) {
    cmem_free(darr_data[i]);
  }
  darray_destroy(darr);
}
