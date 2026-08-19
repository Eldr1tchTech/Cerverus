#include "request.h"

#include "core/containers/string.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"

#include <stddef.h>
#include <sys/types.h>

#define REQUEST_PARSE_INVALID INT_MIN

// TODO: Properly handle http_method_unknown parsing
http_method parse_http_method(const string raw_method) {
  if (string_equal_literal(raw_method, "GET")) {
    return http_method_get;
  } else if (string_equal_literal(raw_method, "HEAD")) {
    return http_method_head;
  } else if (string_equal_literal(raw_method, "OPTIONS")) {
    return http_method_options;
  } else if (string_equal_literal(raw_method, "TRACE")) {
    return http_method_trace;
  } else if (string_equal_literal(raw_method, "PUT")) {
    return http_method_put;
  } else if (string_equal_literal(raw_method, "DELETE")) {
    return http_method_delete;
  } else if (string_equal_literal(raw_method, "POST")) {
    return http_method_post;
  } else if (string_equal_literal(raw_method, "PATCH")) {
    return http_method_patch;
  } else if (string_equal_literal(raw_method, "CONNECT")) {
    return http_method_connect;
  }
  LOG_DEBUG("parse_http_method - Unable to parse an http_method from the "
            "provided string: %s.",
            raw_method);
  return http_method_unknown;
}

http_version parse_http_version(string raw_version) {
  if (string_equal_literal(raw_version, "HTTP/1.1")) {
    return http_version_1p1;
  }
  LOG_DEBUG("parse_http_version - Unable to parse an http_version from the "
            "provided string: %s.",
            raw_version);
  return http_version_unknown;
}

void parse_request_line(request *req, string raw_req_lin) {
  string raw_method = string_empty();
  string raw_version = string_empty();

  string_parse_format(raw_req_lin, "%s %s %s", &raw_method,
                      &req->request_line.URI, &raw_version);

  req->request_line.method = parse_http_method(raw_method);
  string_destroy(raw_method);
  req->request_line.version = parse_http_version(raw_version);
  string_destroy(raw_version);
}

void parse_headers(request *req, string raw_headers) {
  req->headers = darray_create(16, sizeof(header));

  req->headers->length = 0;
  if (string_get_length(raw_headers) == 0) {
    return;
  }

  darray *raw_headers_darr = string_split_at_literal(raw_headers, "\r\n");

  string *raw_headers_darr_data = raw_headers_darr->data;
  for (size_t i = 0; i < raw_headers_darr->length; i++) {

    header new_header;
    string_parse_format(raw_headers_darr_data[i], "%s: %s", &new_header.name,
                        &new_header.value);
    darray_add(req->headers, &new_header);

    string_destroy(raw_headers_darr_data[i]);
  }

  darray_destroy(raw_headers_darr);
}

// TODO: Malformed/Malicious request handling.
// TODO: Resolve memory leak, destroy raw_req before returning in any path.
int request_parse(request *req, char *raw_req, size_t req_len) {
  raw_req = string_create(raw_req);
  int header_terminator = _string_find_literal(raw_req, "\r\n\r\n");
  if (header_terminator == -1) {
    if (req_len >= 1892 - 1) // TODO: revisit this
    {
      return -1; // Malformed, headers should be kept below 1892
    } else {
      return 0; // Unfinished.
    }
  } else {
    // STATUS LINE
    string raw_req_lin = string_split_literal(raw_req, "\r\n");
    parse_request_line(req, raw_req_lin);
    string_destroy(raw_req_lin);

    // handle malformed.
    http_method method = req->request_line.method; // for later
    if (method == http_method_unknown ||
        req->request_line.version == http_version_unknown) {
      return -1; // Malformed.
    }

    // HEADERS
    string raw_headers = string_split_literal(raw_req, "\r\n\r\n");
    parse_headers(req, raw_headers);
    string_destroy(raw_headers); // WARN: unsure as to whether this is
                                 // destroying one of the headers...

    char *content_length_header_value =
        request_get_header_value(req, "Content-Length");

    if (method == http_method_get || method == http_method_head ||
        method == http_method_trace) {
      // No body should be present
      if (content_length_header_value) {
        return -1; // Malformed.
      }

      return header_terminator + 4; // NOTE: is this really correct?
    } else {
      char *content_length_header_value =
          request_get_header_value(req, "Content-Length");
      if (content_length_header_value) {

        u64 content_length;
        if (!string_parse_u64(content_length_header_value, &content_length)) {
          string_destroy(raw_req);
          return -1; // invalid Content-Length
        }

        req->body = _string_create_length(raw_req, content_length);

        string_destroy(raw_req);

        return header_terminator + 4 + content_length;
      }
    }
  }
}

void request_destroy(request *req) {
  cmem_free(req->request_line.URI);

  header *headers_darr_data = req->headers->data;
  for (size_t i = 0; i < req->headers->length; i++) {
    string_destroy(headers_darr_data[i].name);
    string_destroy(headers_darr_data[i].value);
  }
  darray_destroy(req->headers);

  if (req->body) {
    string_destroy(req->body);
  }

  cmem_free(req);
}

string request_get_header_value(request *req, char *header_name) {
  header *headers_darr_data = req->headers->data;
  for (size_t i = 0; i < req->headers->length; i++) {
    if (_raw_string_equal_length(
            headers_darr_data[i].name,
            string_get_length(headers_darr_data[i].name), header_name,
            raw_string_length(
                header_name))) // TODO: Eventually make case insensitive
    {
      return headers_darr_data[i].value;
    }
  }
  return NULL;
}
