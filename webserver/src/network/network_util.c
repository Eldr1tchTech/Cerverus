#include "network_util.h"

#include "core/containers/sv.h"
#include "core/util/logger.h"
#include "defines.h"

lit_t content_type_val_helper(sv_t ext) {
  if (sv_eq_lit(ext, "html"))
    return "text/html";
  if (sv_eq_lit(ext, "css"))
    return "text/css";
  if (sv_eq_lit(ext, "js"))
    return "text/javascript";
  if (sv_eq_lit(ext, "json"))
    return "application/json";
  if (sv_eq_lit(ext, "jpg") || sv_eq_lit(ext, "jpeg"))
    return "image/jpeg";
  if (sv_eq_lit(ext, "png"))
    return "image/png";
  if (sv_eq_lit(ext, "gif"))
    return "image/gif";
  if (sv_eq_lit(ext, "webp"))
    return "image/webp";
  if (sv_eq_lit(ext, "svg"))
    return "image/svg+xml";
  if (sv_eq_lit(ext, "ico"))
    return "image/x-icon";

  LOG_ERROR("content_type_val_helper - Currently unsupported file extension: "
            "%.*s. Returning null.",
            (int)ext.len, ext.ptr);
  // Safe default return?
  return "application/octet-stream";
}