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

    while (line != NULL)
    {
        header *h = &req->headers.headers[req->headers.header_count];
        
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
int request_parse(char *raw_req, size_t req_len, request *req)
{
    char *header_terminator = strstr(raw_req, "\r\n\r\n");
    if (header_terminator == NULL)
    {
        if (req_len >= 1892 - 1) // TODO: revisit this
        {
            return -1; // Malformed, headers should be kept below 1892
        }
        else
        {
            return 0; // Unfinished.
        }
    }
    else
    {
        // fill _raw_buff
        req->_raw_buff = cmem_alloc(memory_tag_request, req_len + 1);
        strcpy(req->_raw_buff, raw_req);

        // STATUS LINE
        char *index = strstr(req->_raw_buff, "\r\n");
        *index = '\0';
        parse_request_line(req, req->_raw_buff);
        char *req_cursor = index + 2;

        // handle malformed.
        http_method method = req->request_line.method; // for later
        if (method == http_method_unknown || req->request_line.version == http_version_unknown)
        {
            return -1; // Malformed.
        }

        // HEADERS
        index = strstr(req_cursor, "\r\n\r\n");
        *index = '\0';
        parse_headers(req, req_cursor);
        req_cursor = index + 4;

        char *content_length_header_value = request_get_header_value(req, "Content-Length");

        if (method == http_method_get || method == http_method_head || method == http_method_trace)
        {
            // No body should be present
            if (content_length_header_value)
            {
                return -1; // Malformed.
            }

            return req_cursor - req->_raw_buff; // Success, change this eventually to flush all pending requests in buffer.
        }
        else
        {
            char *content_length_header_value = request_get_header_value(req, "Content-Length");
            if (content_length_header_value)
            {
                char *endptr = NULL;
                unsigned long content_length = strtoul(content_length_header_value, &endptr, 10);

                if (endptr == content_length_header_value || *endptr != '\0')
                {
                    return -1; // invalid Content-Length
                }

                req->body.data = cmem_alloc(memory_tag_request, content_length + 1);
                req->body.body_size = (size_t)content_length;

                memcpy(req->body.data, req_cursor, content_length);
                req->body.data[content_length] = '\0';

                return (req_cursor - req->_raw_buff) + content_length;
            }
        }
    }
}

void request_destroy(request *req)
{
    cmem_free(memory_tag_request, req->request_line.URI);
    if (req->body.body_size != 0)
        cmem_free(memory_tag_request, req->body.data);
    cmem_free(memory_tag_request, req->_raw_buff);
    cmem_free(memory_tag_request, req);
}

char *request_get_header_value(request *req, char *header_name)
{
    for (size_t i = 0; i < req->headers.header_count; i++)
    {
        if (strcmp(req->headers.headers[i].name, header_name) == 0) // TODO: Eventually make case insensitive
        {
            return req->headers.headers[i].value;
        }
    }
    return NULL;
}