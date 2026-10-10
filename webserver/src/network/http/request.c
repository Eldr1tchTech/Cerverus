#include "request.h"

#include "core/containers/darray.h"
#include "core/containers/sv.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/vendor/sds.h"
#include "network/network_types.inl"

#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#define REQUEST_PARSE_INVALID INT_MIN

// TODO: Properly handle http_method_unknown parsing
static http_method parse_http_method(const sv_t m) {
  if (sv_eq_lit(m, "GET"))
    return http_method_get;
  if (sv_eq_lit(m, "HEAD"))
    return http_method_head;
  if (sv_eq_lit(m, "OPTIONS"))
    return http_method_options;
  if (sv_eq_lit(m, "TRACE"))
    return http_method_trace;
  if (sv_eq_lit(m, "PUT"))
    return http_method_put;
  if (sv_eq_lit(m, "DELETE"))
    return http_method_delete;
  if (sv_eq_lit(m, "POST"))
    return http_method_post;
  if (sv_eq_lit(m, "PATCH"))
    return http_method_patch;
  if (sv_eq_lit(m, "CONNECT"))
    return http_method_connect;

  LOG_DEBUG("parse_http_method - Unable to parse an http_method from the "
            "provided string: %.*s.",
            (int)m.len, m.str);
  return http_method_unknown;
}

static http_version parse_http_version(const sv_t v) {
  if (sv_eq_lit(v, "HTTP/1.1"))
    return http_version_1p1;

  LOG_DEBUG("parse_http_version - Unable to parse an http_version from the "
            "provided string: %.*s.",
            (int)v.len, v.str);
  return http_version_unknown;
}

static bool parse_status_line(request *req, sv_t line) {
  const char *end = line.str + line.len;

  const char *sp1 = memchr(line.str, ' ', line.len);
  if (!sp1)
    return false;

  const char *sp2 = memchr(sp1 + 1, ' ', end - (sp1 + 1));
  if (!sp2)
    return false;

  sv_t method = {line.str, (size_t)(sp1 - line.str)};
  sv_t uri = {sp1 + 1, (size_t)(sp2 - (sp1 + 1))};
  sv_t version = {sp2 + 1, (size_t)(end - (sp2 + 1))};

  if (method.len == 0 || uri.len == 0)
    return false; // double-space
  if (memchr(version.str, ' ', version.len))
    return false; // 4th token

  req->status_line.method = parse_http_method(method);
  req->status_line.URI = uri;
  req->status_line.version = parse_http_version(version);

  return req->status_line.method != http_method_unknown &&
         req->status_line.version != http_version_unknown;
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
