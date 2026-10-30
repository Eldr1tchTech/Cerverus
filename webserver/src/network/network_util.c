#include "network_util.h"

#include "core/util/logger.h"
#include "core/vendor/sds.h"
#include "defines.h"

lit_t content_type_val_helper(sds ext) {
  if (ext) {

    // TODO: write a helper function for the lit wrapper here
    if (sdscmplit(ext, "html")) {
      return "text/html";
    } else if (sdscmplit(ext, "css")) {
      return "text/css";
    } else if (sdscmplit(ext, "jpg") || sdscmplit(ext, "jpeg")) {
      return "image/jpeg";
    } else if (sdscmplit(ext, "png")) {
      return "image/png";
    } else if (sdscmplit(ext, "gif")) {
      return "image/gif";
    } else if (sdscmplit(ext, "webp")) {
      return "image/webp";
    } else if (sdscmplit(ext, "svg")) {
      return "image/svg+xml";
    } else if (sdscmplit(ext, "ico")) {
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