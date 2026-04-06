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

/*
List of all possible states the request can be in:
- no double CRLF and reqlen is not buffer size (1982 rn)
    - keep filling buffer
- no double CRLF and reqlen is buffer size (1982 rn)
    - fatal error, this is an malicious request (probably)
- double CRLF is present and it doesn't have a body (based on http_method and content-length header presence)
    - parse headers, return amount of bytes consumed
- double CRLF is present and it has a body
    - the entire body is there
        - parse it in
    - the entire body is not there
        - return amount of consumed bytes, up to the double CRLF, store that body is present somewhere, on next pass repeat this check,
*/

// TODO: Malformed/Malicious request handling.
// TODO: Eventually get rid of buffer duplication because now a copy is being saved in the request as well as the context structure.
// TODO: I don't think I can actually modify raw_req anymore, so need to refactor that as well.
request_parse_state_context request_parse(char *raw_req, ssize_t reqlen, request **req)
{
    int b_consumed = 0;
    request* _req;
    char *index = raw_req;

    if ((*req)->_raw_buff == NULL)
    {
        char *header_terminator = strstr(raw_req, "\r\n\r\n");
        if (header_terminator == NULL)
        {
            // TODO: Special case if reqlen is 1892
            return (request_parse_state_context){
                .type = request_parse_state_unfinished,
            };
        }

        *req = cmem_alloc(memory_tag_request, sizeof(request));
        _req = *req;

        _req->_raw_buff = cmem_alloc(memory_tag_request, strlen(raw_req) + 1); // Do I actually need the plus one? isn't it null terminated by default?
        strcpy(_req->_raw_buff, raw_req);

        char *raw_req_start = raw_req;

        // STATUS LINE
        index = strstr(raw_req, "\r\n");
        *index = '\0';
        parse_request_line(req, raw_req);
        raw_req = index + 2;

        // handle http_method_unknown
        http_method method = _req->request_line.method;
        if (method == http_method_unknown)
        {
            return (request_parse_state_context){
                .type = request_parse_state_invalid,
            };
        }

        // handle http_version_unknown
        http_version version = _req->request_line.version;
        if (version == http_version_unknown)
        {
            return (request_parse_state_context){
                .type = request_parse_state_invalid,
            };
        }

        // HEADERS
        index = strstr(raw_req, "\r\n\r\n");
        *index = '\0';
        parse_headers(req, raw_req);
        raw_req = index + 4;

        b_consumed = raw_req - raw_req_start;

        if (method == http_method_get || method == http_method_head || method == http_method_trace)
        {
            return (request_parse_state_context){
                .type = request_parse_state_succeded,
                .bytes_consumed = b_consumed,
            };
        }
    }

    if (!_req)
    {
        _req = *req;
    }

    // BODY
    // Get header value
    char *raw_content_length = request_get_header_value(*req, "Content-Length");
    if (raw_content_length == NULL)
    {
        // TODO: For now just invalid, probably a better way to handle
        return (request_parse_state_context){
            .type = request_parse_state_invalid, // Maybe do some cleanup?
        };
    }

    // Convert header value
    char *end;
    long content_length = strtol(raw_content_length, &end, 10);
    if (end == raw_content_length)
    {
        // no digits found — malformed header
        return (request_parse_state_context){
            .type = request_parse_state_invalid, // Maybe do some cleanup?
        };
    }

    // Check content-length vs buffer
    if (reqlen - (b_consumed) >= content_length)
    {
        _req->body.data = cmem_alloc(memory_tag_request, content_length + 1);
        cmem_mcpy(_req->body.data, index, content_length);

        return (request_parse_state_context){
            .type = request_parse_state_succeded,
            .bytes_consumed = b_consumed + content_length,
        };
    }
    else
    {
        return (request_parse_state_context){
            .type = request_parse_state_unfinished,
            .bytes_consumed = b_consumed,
        };
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
        if (strcmp(req->headers.headers[i].name, "Content-Length") == 0) // TODO: Eventually make case insensitive
        {
            return req->headers.headers[i].value;
        }
    }
    return NULL;
}