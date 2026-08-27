#include "network_util.h"

#include "core/containers/string.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"

string *parse_URI(string URI) { return str_split_at_lit(URI, "/"); }

str_lit content_type_val_helper(string ext) {
  if (ext) {

    if (str_equal_lit(ext, "html")) {
      return "text/html";
    } else if (str_equal_lit(ext, "css")) {
      return "text/css";
    } else if (str_equal_lit(ext, "jpg") || str_equal_lit(ext, "jpeg")) {
      return "image/jpeg";
    } else if (str_equal_lit(ext, "png")) {
      return "image/png";
    } else if (str_equal_lit(ext, "gif")) {
      return "image/gif";
    } else if (str_equal_lit(ext, "webp")) {
      return "image/webp";
    } else if (str_equal_lit(ext, "svg")) {
      return "image/svg+xml";
    } else if (str_equal_lit(ext, "ico")) {
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