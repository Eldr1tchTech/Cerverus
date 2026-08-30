#include "request.h"

#include "core/containers/string.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"

#include <stddef.h>
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

http_version parse_http_version(string raw_version) {
  if (str_equal_lit(raw_version, "HTTP/1.1")) {
    return http_version_1p1;
  }
  LOG_DEBUG("parse_http_version - Unable to parse an http_version from the "
            "provided string: %s.",
            raw_version);
  return http_version_unknown;
}

void parse_request_line(request *req, string raw_req_lin) {
  string raw_method = str_empty();
  string raw_version = str_empty();

  str_parse_fmt(raw_req_lin, "%s %s %s", &raw_method, &req->request_line.URI,
                &raw_version);

  req->request_line.method = parse_http_method(raw_method);
  str_destroy(raw_method);
  req->request_line.version = parse_http_version(raw_version);
  str_destroy(raw_version);
}

void parse_headers(request *req, string raw_headers) {
  req->headers = darray_create(16, sizeof(header));

  *darray_get_length(req->headers) = 0;
  if (str_get_len(raw_headers) == 0) {
    return;
  }

  string *raw_headers_darr = str_split_at_lit(raw_headers, "\r\n");
  for (size_t i = 0; i < *darray_get_length(raw_headers_darr); i++) {

    header new_header;
    str_parse_fmt(raw_headers_darr[i], "%s: %s", &new_header.name,
                  &new_header.value);
    darray_add(req->headers, &new_header);

    str_destroy(raw_headers_darr[i]);
  }

  darray_destroy(raw_headers_darr);
}

// TODO: Malformed/Malicious request handling.
// TODO: Resolve memory leak, destroy raw_req before returning in any path.
int request_parse(request *req, char *raw_req, size_t req_len) {
  raw_req = str_create(raw_req);
  int header_terminator = _str_find_lit(raw_req, "\r\n\r\n");
  if (header_terminator == -1) {
    if (req_len >= 1892 - 1) // TODO: revisit this
    {
      return -1; // Malformed, headers should be kept below 1892
    } else {
      return 0; // Unfinished.
    }
  } else {
    // STATUS LINE
    string raw_req_lin = str_split_lit(raw_req, "\r\n");
    parse_request_line(req, raw_req_lin);
    str_destroy(raw_req_lin);

    // handle malformed.
    http_method method = req->request_line.method; // for later
    if (method == http_method_unknown ||
        req->request_line.version == http_version_unknown) {
      return -1; // Malformed.
    }

    // HEADERS
    string raw_headers = str_split_lit(raw_req, "\r\n\r\n");
    parse_headers(req, raw_headers);
    str_destroy(raw_headers); // WARN: unsure as to whether this is
                              // destroying one of the headers...

    char *content_length_header_value =
        request_get_header_value(req, "Content-Length");

    if (method == http_method_get || method == http_method_head ||
        method == http_method_trace) {
      // No body should be present
      if (content_length_header_value) {
        return -1; // Malformed.
      }

      return header_terminator + 4;
    } else {
      char *content_length_header_value =
          request_get_header_value(req, "Content-Length");
      if (content_length_header_value) {

        u64 content_length;
        if (!str_parse_u64(content_length_header_value, &content_length)) {
          str_destroy(raw_req);
          return -1; // invalid Content-Length
        }

        req->body = _str_create_len(raw_req, content_length);

        str_destroy(raw_req);

        return header_terminator + 4 + content_length;
      }
      return -1; // TODO: WARN: This is currently just a placeholder.
    }
  }
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

string request_get_header_value(request *req, char *header_name) {
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
