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
    return NULL;
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

char *response_serialize(response *res) {
  // Pass 1: Calculate size
  size_t size = 0;

  const char *version = serialize_http_version(res->status_line.version);
  if (!version) {
    LOG_ERROR("response_serialize - No version value.");
    return NULL;
  }

  // STATUS LINE

  // HEADERS
  header *headers_darr_data = res->headers->data;
  for (size_t i = 0; i < res->headers->length; i++) {
    size +=
        snprintf(NULL, 0, "%s: %s\r\n", headers_darr_data[i].name,
                 headers_darr_data[i].value); // TODO: Finish replacing snprintf
  }

  // BODY

  // Pass 2: Allocate string and fill it

  // Allocate
  char *raw_res = cmem_alloc((size) * sizeof(char));
  size_t offset = 0;

  // STATUS LINE
  offset += snprintf(
      raw_res + offset, size - offset, "%s %i %s\r\n", version,
      res->status_line.status_code,
      res->status_line
          .reason_phrase); // Don't really need the + offset in the first field
                           // since it's the first thing being added...

  // HEADERS
  for (size_t i = 0; i < res->headers->length; i++) {
    offset += snprintf(raw_res + offset, size - offset, "%s: %s\r\n",
                       headers_darr_data[i].name, headers_darr_data[i].value);
  }

  // BODY
  // TODO: body handling has been removed for now, as there is no need until the
  // server can at least serve static files.

  return raw_res;
}
