#include "request.h"

#include "core/containers/darray.h"
#include "core/containers/sv.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/vendor/sds.h"
#include "defines.h"
#include "network/network_types.inl"

#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#define REQUEST_PARSE_INVALID INT_MIN

// TODO: Properly handle http_method_unknown parsing
http_method parse_http_method(const string raw_method) {
  if (str_equal_lit(raw_method, "GET")) {
    return http_method_get;
  } else if (str_equal_lit(raw_method, "HEAD")) {
    return http_method_head;
  } else if (str_equal_lit(raw_method, "OPTIONS")) {
    return http_method_options;
  } else if (str_equal_lit(raw_method, "TRACE")) {
    return http_method_trace;
  } else if (str_equal_lit(raw_method, "PUT")) {
    return http_method_put;
  } else if (str_equal_lit(raw_method, "DELETE")) {
    return http_method_delete;
  } else if (str_equal_lit(raw_method, "POST")) {
    return http_method_post;
  } else if (str_equal_lit(raw_method, "PATCH")) {
    return http_method_patch;
  } else if (str_equal_lit(raw_method, "CONNECT")) {
    return http_method_connect;
  }
  LOG_DEBUG("parse_http_method - Unable to parse an http_method from the "
            "provided string: %s.",
            raw_method);
  return http_method_unknown;
}

http_version parse_http_version(sds raw_version) {
  if (sdseqlit(raw_version, "HTTP/1.1")) {
    return http_version_1p1;
  }
  LOG_DEBUG("parse_http_version - Unable to parse an http_version from the "
            "provided string: %s.",
            raw_version);
  return http_version_unknown;
}

static bool parse_status_line(request *req, sv_t raw_req_lin) {
  str_parse_fmt(raw_req_lin, "%s %s %s", &raw_method, &req->request_line.URI,
                &raw_version);

  req->request_line.method = parse_http_method(raw_method);
  sds_free(raw_method);
  req->request_line.version = parse_http_version(raw_version);
  sds_free(raw_version);
}

static bool parse_headers(request *req, sv_t raw_hdrs) {
  req->headers = darray_create(16, sizeof(header));
  const char *p = raw_hdrs.str, *end = raw_hdrs.str + raw_hdrs.len;

  while (p < end) {
    const char *eol = memmem(p, end - p, "\r\n", 2);
    if (!eol)
      eol = end;

    const char *colon = memchr(p, ':', eol - p);
    if (!colon || colon == p)
      return false;

    const char *v = colon + 1, *ve = eol;
    while (v < ve && (*v == ' ' || *v == '\t'))
      v++;
    while (ve > v && (*v == ' ' || *v == '\t'))
      ve--;

    // TODO: how exactly do const qualifiers work? are they only for that scope?
    header h = {*sv_create(p, colon - p), *sv_create(v, ve - v)};
    req->headers = darray_add(req->headers, &h);
    p = eol + 2;
  }
  return true;
}

// TODO: Malformed/Malicious request handling.
// TODO: Resolve memory leak, destroy raw_req before returning in any path.
int request_parse(request *req) {
  char *hdr_end = memmem(req->_raw.str, req->_raw.len, "\r\n\r\n", 4);
  if (hdr_end)
    return req->_raw.len >= 1891 ? -1 : 0;

  // Status Line
  const char *line_end =
      memmem(req->_raw.str, hdr_end - req->_raw.str + 2, "/r/n", 2);

  if (!parse_status_line(req, (sv_t){.str = req->_raw.str,
                                     .len = (line_end + 2) - (req->_raw.str)}))
    return -1;

  // Headers
  if (!parse_headers(req, (sv_t){.str = line_end + 2,
                                 .len = (hdr_end + 2) - (line_end + 2)}))
    return -1;

  // Body
}

void request_destroy(request *req) {
  cmem_free(req->request_line.URI);

  for (size_t i = 0; i < *darray_get_length(req->headers); i++) {
    str_destroy(req->headers[i].name);
    str_destroy(req->headers[i].value);
  }
  darray_destroy(req->headers);

  if (req->body) {
    str_destroy(req->body);
  }

  cmem_free(req);
}

sds request_get_header_value(request *req, char *header_name) {
  for (size_t i = 0; i < *darray_get_length(req->headers); i++) {
    if (_raw_str_equal_len(
            req->headers[i].name, str_get_len(req->headers[i].name),
            header_name,
            raw_str_len(header_name))) // TODO: Eventually make case
                                       // insensitiveheaders_darr_data
    {
      return req->headers[i].value;
    }
  }
  return nullptr;
}
