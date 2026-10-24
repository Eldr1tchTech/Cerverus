#include "response.h"

#include "core/containers/darray.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/util/util.h"
#include "core/vendor/sds.h"
#include "defines.h"
#include "network/network_types.inl"

#include <stdio.h>

lit_t serialize_http_version(http_version version) {
  switch (version) {
  case http_version_1p1:
    return HTTP_VERSION_1P1_LITERAL;

  default:
    LOG_ERROR("Unable to serialize unknown http_version.");
    return nullptr;
  }
}

response_t *response_create() {
  response_t *new_res = cmem_alloc(sizeof(response_t));

  new_res->headers = darray_create(16, sizeof(header_t));

  return new_res;
}

void response_add_header(response_t *res, header_t h) {
  darray_add(res->headers, &h);
}

void response_destroy(response_t* res) {
  // STATUS LINE

  // HEADERS
  darray_foreach(res->headers, elem) {
    sds_free(elem->sds.name);
    sds_free(elem->sds.value);
  }
  darray_destroy(res->headers);

  // BODY
  if (res->body) {
    sds_free(res->body);
  }
}

sds response_serialize(response_t *res) {
  // Pass 1: Calculate size
  size_t size = 0;

  // STATUS LINE
  lit_t version = serialize_http_version(res->status_line.version);
  if (!version) {
    LOG_ERROR("response_serialize - No version value.");
    return nullptr;
  }

  size += litlen(version) + 1 +
          str_get_u64_len(res->status_line.status_code) + 1 +
          str_get_len(res->status_line.reason_phrase) + 2;

  // HEADERS
  for (size_t i = 0; i < *darray_get_length(res->headers); i++) {
    size += sdslen(res->headers[i].sds.name) + 2 +
            sdslen(res->headers[i].sds.value) + 2;
  }

  size += 2;

  // BODY

  // Pass 2: Allocate string and fill it

  // Allocate
  sds raw_res = sdsempty();

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
