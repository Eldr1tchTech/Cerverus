#include "response.h"

#include "core/containers/darray.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/vendor/sds.h"
#include "defines.h"
#include "network/network_types.inl"
#include <string.h>

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

  // BODY
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
  sds raw_res = sdsempty();

  lit_t version = serialize_http_version(res->status_line.version);
  if (version == nullptr) return raw_res;

  // STATUS LINE
  raw_res = sdscatprintf(raw_res, "%s %i %s\r\n", version, res->status_line.status_code, res->status_line.reason_phrase);

  // HEADERS
  darray_foreach(res->headers, elem) {
    raw_res = sdscatprintf(raw_res, "%s: %s\r\n", elem->sds.name, elem->sds.value);
  }

  // BODY
  if (res->body) raw_res = sdscatsds(raw_res, res->body);

  // Destroy response
  response_destroy(res);

  return raw_res;
}
