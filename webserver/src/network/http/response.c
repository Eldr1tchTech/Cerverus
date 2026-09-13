#include "response.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"

#include <stdio.h>

// Use string literals for performance boosts
#define HTTP_VERSION_1P1_LITERAL "HTTP/1.1"

char *serialize_http_version(http_version version) {
  switch (version) {
  case http_version_1p1:
    return HTTP_VERSION_1P1_LITERAL;

  default:
    LOG_ERROR("Unable to serialize unknown http_version.");
    return nullptr;
  }
}

response *response_create() {
  response *new_res = cmem_alloc(sizeof(response));

  new_res->headers = darray_create(16, sizeof(header));

  return new_res;
}

void response_add_header(response *res, header h) {
  darray_add(res->headers, &h);
}

string response_serialize(response *res) {
  // Pass 1: Calculate size
  size_t size = 0;

  // STATUS LINE
  const char *version = serialize_http_version(res->status_line.version);
  if (!version) {
    LOG_ERROR("response_serialize - No version value.");
    return nullptr;
  }

  size += STR_LIT_LEN(version) + 1 +
          str_get_u64_len(res->status_line.status_code) + 1 +
          str_get_len(res->status_line.reason_phrase) + 2;

  // HEADERS
  for (size_t i = 0; i < *darray_get_length(res->headers); i++) {
    size += str_get_len(res->headers[i].name) + 2 +
            str_get_len(res->headers[i].value) + 2;
  }

  size += 2;

  // BODY

  // Pass 2: Allocate string and fill it

  // Allocate
  string raw_res = str_empty();
  raw_res = str_grow_to(raw_res, size);

  // STATUS LINE
  str_cat_str_lit(raw_res, version);
  str_cat_str_lit(raw_res, " ");
  str_cat_u64(raw_res, res->status_line.status_code);
  str_cat_str_lit(raw_res, " ");
  str_cat_str(raw_res, res->status_line.reason_phrase);
  str_cat_str_lit(raw_res, "\r\n");

  // HEADERS
  for (size_t i = 0; i < *darray_get_length(res->headers); i++) {
    str_cat_str(raw_res, res->headers[i].name);
    str_cat_str_lit(raw_res, ": ");
    str_cat_str(raw_res, res->headers[i].value);
    str_cat_str_lit(raw_res, "\r\n");
  }

  str_cat_str_lit(raw_res, "\r\n");

  // BODY
  // TODO: body handling has been removed for now, as there is no need until the
  // server can at least serve static files.

  // Destroy response
  str_destroy(res->status_line.reason_phrase);
  for (size_t i = 0; i < *darray_get_length(res->headers); i++) {
    str_destroy(res->headers[i].name);
    str_destroy(res->headers[i].value);
  }

  return raw_res;
}
