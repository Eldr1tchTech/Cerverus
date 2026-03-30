#pragma once

#include "defines.h"

#include "core/containers/darray.h"

#define MAX_HEADER_COUNT 16

typedef enum http_method {
    http_method_get,
    http_method_head,
    http_method_options,
    http_method_trace,
    http_method_put,
    http_method_delete,
    http_method_post,
    http_method_patch,
    http_method_connect,
    http_method_unknown,
} http_method;

typedef enum http_version {
    http_version_1p1,
    http_version_unknown,
} http_version;

typedef struct header
{
    char* name;
    char* value;
} header;

typedef struct request
{
    char* _raw_buff;
    struct {
        http_method method;
        char* URI;
        http_version version;
    } request_line;
    struct
    {
        header headers[MAX_HEADER_COUNT];
        size_t header_count;
    } headers;
    struct {
        char* data;
        size_t body_size;
    } body;
} request;

typedef struct response
{
    struct {
        http_version version;
        int status_code;
        char* reason_phrase;
    } status_line;
    struct
    {
        header headers[MAX_HEADER_COUNT];
        size_t header_count;
    } headers;
    struct {
        char* data;
        size_t body_size;
    } body;
} response;

typedef void (*route_callback)(request* req, int client_fd);

typedef struct route_segment {
    char* path_segment;
    bool is_dynamic;
} route_segment;

typedef struct route {
    darray* segments;
    http_method method;
    route_callback callback;
};


typedef struct trie_node
{
    darray* children;
    route_segment segment;
    route_callback callback;
} trie_node;

typedef struct trie
{
    trie_node** roots;
} trie;