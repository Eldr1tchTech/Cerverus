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

char *response_serialize(response *res)
{
    char *raw_res = cmem_alloc(memory_tag_response, 8192 * sizeof(char));
    snprintf(raw_res, 8192 * sizeof(char),
             "%s %i %s\r\n",
             serialize_http_version(res->status_line.version),
             res->status_line.status_code, res->status_line.reason_phrase);
    
    char raw_header[521] = {0}; 
    for (size_t i = 0; i < res->headers.header_count; i++)
    {
        snprintf(raw_header, 512, "%s: %s\r\n", res->headers.headers[i].name, res->headers.headers[i].value); 
        strcat(raw_res, raw_header);
    }

    strcat(raw_res, "\r\n");

    if (res->body.body_size != 0)
    {
        strncat(raw_res, res->body.data, res->body.body_size);
        strcat(raw_res, "\r\n");
    }
    
    return raw_res;
}

char* content_type_val_helper(const char* ext) {
    if (ext)
    {
        if (strcmp(ext + 1, "html") == 0)
        {
            return "text/html";
        } else if (strcmp(ext + 1, "css") == 0)
        {
            return "text/css";
        } else {
            LOG_ERROR("content_type_val_helper - Currently unsuported file extension: %s. Returning null.", ext);
            return NULL;
        }
    }
    LOG_ERROR("content_type_val_helper - Please provide a valid char* for ext.");
    return NULL;
}