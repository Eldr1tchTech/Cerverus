#include "response.h"

#include "core/util/logger.h"
#include "core/memory/cmem.h"

#include <stdio.h>
#include <string.h>

char *serialize_http_version(http_version version)
{
    switch (version)
    {
    case http_version_1p1:
        return "HTTP/1.1";
    default:
        LOG_ERROR("Unable to serialize unknown http_version.");
        return NULL;
    }
}

response *response_create(int b_size)
{
    response *new_res = cmem_alloc(memory_tag_response, sizeof(response));
    new_res->body.data = cmem_alloc(memory_tag_response, (b_size + 1) * sizeof(char)); // + 1 to account for null terminator
    new_res->body.body_size = b_size;

    return new_res;
}

void response_add_header(response* res, header h) {
    if (res->headers.header_count == MAX_HEADER_COUNT)
    {
        LOG_ERROR("response_add_header - Headers already at MAX_HEADER_COUNT. Failing.");
        return;
    }
    
    res->headers.headers[res->headers.header_count++] = h;
}

char *response_serialize(response *res)
{
    // Pass 1: Calculate size
    size_t size = 0;

    const char *version = serialize_http_version(res->status_line.version);
    if (!version)
    {
        LOG_ERROR("response_serialize - No version value.");
        return NULL;
    }

    // STATUS LINE
    size += snprintf(NULL, 0, "%s %i %s\r\n",
                    serialize_http_version(res->status_line.version),
                    res->status_line.status_code,
                    res->status_line.reason_phrase);

    // HEADERS
    for (size_t i = 0; i < res->headers.header_count; i++)
    {
        size += snprintf(NULL, 0, 
                        "%s: %s\r\n",
                        res->headers.headers[i].name,
                        res->headers.headers[i].value);
    }

    size += snprintf(NULL, 0, "\r\n");

    // BODY
    size += res->body.body_size + 1; // For null terminator

    // Pass 2: Allocate string and fill it

    // Allocate
    char *raw_res = cmem_alloc(memory_tag_response, (size) * sizeof(char));
    size_t offset = 0;

    // STATUS LINE
    offset += snprintf(raw_res + offset, size - offset,
                       "%s %i %s\r\n",
                       version,
                       res->status_line.status_code,
                       res->status_line.reason_phrase); // Don't really need the + offset in the first field since it's the first thing being added...

    // HEADERS
    for (size_t i = 0; i < res->headers.header_count; i++)
    {
        offset += snprintf(raw_res + offset, size - offset,
                           "%s: %s\r\n",
                           res->headers.headers[i].name,
                           res->headers.headers[i].value);
    }

    offset += snprintf(raw_res + offset, size - offset, "\r\n");

    // BODY
    if (res->body.body_size > 0)
    {
        cmem_mcpy(raw_res + offset, res->body.data, res->body.body_size);
        offset += res->body.body_size;
    }

    cmem_free(memory_tag_response, res->body.data);
    cmem_free(memory_tag_response, res);

    return raw_res;
}