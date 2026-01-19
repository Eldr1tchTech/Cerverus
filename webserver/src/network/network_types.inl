#pragma once

#include "defines.h"

typedef enum http_method {
    http_method_get,
} http_method;

typedef enum http_version {
    http_version_1p1,
} http_version;

typedef struct header
{
    char* name;
    char* value;
} header;

typedef struct request
{
    struct {
        http_method method;
        char* URI;
        http_version version;
    } request_line;
    struct {
        header* headers;
        int header_count;
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
    struct {
        header* headers;
        int header_count;
    } headers;
    struct {
        char* data;
        size_t body_size;
    } body;
} response;
