#include "network_util.h"

#include "core/containers/sv.h"
#include "core/util/logger.h"
#include "core/vendor/sds.h"
#include "defines.h"

lit_t content_type_val_helper(sv_t ext) {
    // TODO: write a helper function for the lit wrapper here
    if (svcmplit(ext, "html")) {
      return "text/html";
    } else if (svcmplit(ext, "css")) {
      return "text/css";
    } else if (svcmplit(ext, "jpg") || svcmplit(ext, "jpeg")) {
      return "image/jpeg";
    } else if (svcmplit(ext, "png")) {
      return "image/png";
    } else if (svcmplit(ext, "gif")) {
      return "image/gif";
    } else if (svcmplit(ext, "webp")) {
      return "image/webp";
    } else if (svcmplit(ext, "svg")) {
      return "image/svg+xml";
    } else if (svcmplit(ext, "ico")) {
      return "image/x-icon";
    } else {
      LOG_ERROR("content_type_val_helper - Currently unsuported file "
                "extension: %s. Returning null.",
                ext);
      return nullptr;
    }
  LOG_ERROR("content_type_val_helper - Please provide a valid char* for ext.");
  return nullptr;
}