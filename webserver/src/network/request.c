#include "request.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"

#include <string.h>

http_method parse_http_method(char *raw_method)
{
    if (strcmp(raw_method, "GET") == 0)
    {
        return http_method_get;
    }
    LOG_DEBUG("parse_http_method - Unable to parse an http_method from the provided string: %s.", raw_method);
    return http_method_unknown;
}

http_version parse_http_version(char *raw_version)
{
    if (strcmp(raw_version, "HTTP/1.1") == 0)
    {
        return http_version_1p1;
    }
    LOG_DEBUG("parse_http_version - Unable to parse an http_version from the provided string: %s.", raw_version);
    return http_version_unknown;
}

void parse_request_line(request *req, char *raw_req_lin)
{
    req->request_line.method = parse_http_method(strtok(raw_req_lin, " "));

    char* URI_val = strtok(NULL, " ");
    req->request_line.URI = cmem_alloc(memory_tag_request, (strlen(URI_val) + 1) * sizeof(char));
    strcpy(req->request_line.URI, URI_val);

    req->request_line.version = parse_http_version(strtok(NULL, "\0"));
}

void parse_headers(request *req, char *raw_headers)
{
    if (!raw_headers || strlen(raw_headers) == 0)
    {
        req->headers = NULL;
        return;
    }

    char *line = strtok(raw_headers, "\r\n");
    header h;

    while (line != NULL)
    {
        // Find the colon separator
        char *colon = strchr(line, ':');
        if (colon != NULL)
        {
            // Split at the colon
            *colon = '\0';

            // Header name is everything before the colon
            h.name = line;

            // Header value is everything after the colon (skip leading space)
            char *value = colon + 1;
            while (*value == ' ')
            {
                value++;
            }
            h.value = value;

            darray_add(req->headers, &h);
        }
        else
        {
            LOG_DEBUG("parse_headers - Malformed header line: %s", line);
        }

        line = strtok(NULL, "\r\n");
    }
}

request *request_parse(char *raw_req)
{
    request *req = cmem_alloc(memory_tag_request, sizeof(request));
    req->headers = darray_create(4, sizeof(header));

    char *raw_req_cpy = cmem_alloc(memory_tag_request, sizeof(char) * 4096);
    strcpy(raw_req_cpy, raw_req);

    req->_raw_buff = raw_req_cpy;

    // STATUS LINE
    char *index = strstr(raw_req_cpy, "\r\n");
    *index = '\0';
    parse_request_line(req, raw_req_cpy);
    raw_req_cpy = index + 2;

    // HEADERS
    index = strstr(raw_req_cpy, "\r\n\r\n");
    *index = '\0';

    parse_headers(req, raw_req_cpy);

    // BODY
    raw_req_cpy = index + 4;
    req->body.body_size = strlen(raw_req_cpy);
    req->body.data = cmem_alloc(memory_tag_request, (req->body.body_size + 1) * sizeof(char));
    strcpy(req->body.data, raw_req_cpy);

    return req;
}

void request_destroy(request *req)
{
    cmem_free(memory_tag_request, req->request_line.URI);
    darray_destroy(req->headers);
    if (req->body.body_size != 0) cmem_free(memory_tag_request, req->body.data);
    cmem_free(memory_tag_request, req->_raw_buff);
    cmem_free(memory_tag_request, req);
    req = 0;
}