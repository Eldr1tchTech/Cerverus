#include "request.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"

#include <string.h>
#include <sys/types.h>

#define REQUEST_PARSE_INVALID INT_MIN

// TODO: Properly handle http_method_unknown parsing
http_method parse_http_method(char *raw_method)
{
    if (strcmp(raw_method, "GET") == 0)
    {
        return http_method_get;
    }
    else if (strcmp(raw_method, "HEAD") == 0)
    {
        return http_method_head;
    }
    else if (strcmp(raw_method, "OPTIONS") == 0)
    {
        return http_method_options;
    }
    else if (strcmp(raw_method, "TRACE") == 0)
    {
        return http_method_trace;
    }
    else if (strcmp(raw_method, "PUT") == 0)
    {
        return http_method_put;
    }
    else if (strcmp(raw_method, "DELETE") == 0)
    {
        return http_method_delete;
    }
    else if (strcmp(raw_method, "POST") == 0)
    {
        return http_method_post;
    }
    else if (strcmp(raw_method, "PATCH") == 0)
    {
        return http_method_patch;
    }
    else if (strcmp(raw_method, "CONNECT") == 0)
    {
        return http_method_connect;
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

    char *URI_val = strtok(NULL, " ");
    req->request_line.URI = cmem_alloc(memory_tag_request, (strlen(URI_val) + 1) * sizeof(char));
    strcpy(req->request_line.URI, URI_val);

    req->request_line.version = parse_http_version(strtok(NULL, "\0"));
}

void parse_headers(request *req, char *raw_headers)
{
    if (!raw_headers || strlen(raw_headers) == 0)
    {
        req->headers.header_count = 0;
        return;
    }

    char *line = strtok(raw_headers, "\r\n");
    req->headers.header_count = 0;
    header *h = &req->headers.headers[req->headers.header_count];

    while (line != NULL)
    {
        // Find the colon separator
        char *colon = strchr(line, ':');
        if (colon != NULL)
        {
            // Split at the colon
            *colon = '\0';

            // Header name is everything before the colon
            h->name = line;

            // Header value is everything after the colon (skip leading space)
            char *value = colon + 1;
            while (*value == ' ')
            {
                value++;
            }
            h->value = value;
            req->headers.header_count++;
        }
        else
        {
            LOG_DEBUG("parse_headers - Malformed header line: %s", line);
        }

        line = strtok(NULL, "\r\n");
    }
}

// TODO: Malformed/Malicious request handling.
// TODO: Eventually get rid of buffer duplication because now a copy is being saved in the request as well as the context structure.
// TODO: I don't think I can actually modify raw_req anymore, so need to refactor that as well.
request_parse_state request_parse(char *raw_req, ssize_t reqlen, request **req)
{
    // TODO: Implement this and add more stringent error checking and handling, as consumption of leftover requests will be more common now...
    // OPTIMIZATION: write a custom searcher, that can take the length as an argument to allow for it to be precomputed.
    // OPTIMIZATION: maybe only search based on an offset, as you already searched the first part in the last pass.
    char *header_terminator = strstr(raw_req, "\r\n\r\n");
    if (header_terminator == NULL)
    {
        // TODO: Special case if reqlen is 1892
        return (request_parse_state) {
            .type = request_parse_state_type_unfinished,
        };
    }

    *req = cmem_alloc(memory_tag_request, sizeof(request));

    request* _req = *req;

    _req->_raw_buff = cmem_alloc(memory_tag_request, strlen(raw_req) + 1);
    strcpy(_req->_raw_buff, raw_req);

    // STATUS LINE
    char *index = strstr(raw_req, "\r\n");
    *index = '\0';
    parse_request_line(req, raw_req);
    raw_req = index + 2;

    // handle http_method_unknown
    http_method method = _req->request_line.method;
    if (method == http_method_unknown)
    {
        return (request_parse_state) {
            .type = request_parse_state_type_invalid,
        };
    }

    // HEADERS
    index = strstr(raw_req, "\r\n\r\n");
    *index = '\0';
    parse_headers(req, raw_req);
    raw_req = index + 4; // TODO: isn't it +2?
    
    if (method == http_method_get || method == http_method_head || method == http_method_trace)
    {
        return (request_parse_state) {
            .type = request_parse_state_type_succeded,
            .bytes_consumed = ,
        };
    }

    // BODY
    
}

void request_destroy(request *req)
{
    cmem_free(memory_tag_request, req->request_line.URI);
    if (req->body.body_size != 0)
        cmem_free(memory_tag_request, req->body.data);
    cmem_free(memory_tag_request, req->_raw_buff);
    cmem_free(memory_tag_request, req);
}