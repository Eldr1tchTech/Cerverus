#pragma once

#include "core/containers/darray.h"
#include "network_types.inl"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// TODO: Change this to use a command buffer of some sort for the server?
typedef void (*route_callback)(request* req, response* res);

typedef struct route
{
    http_method method;
    char* URI;
    route_callback callback;
} route;

typedef struct router_config {
    route_callback default_route;
    bool use_public_directory;
} router_config;

typedef struct router
{
    router_config r_conf;
    darray* routes;
} router;

router* router_create(router_config r_conf);

void router_handle_route(router* r, request* req, response* res);

void router_destroy(router* r);

// This does not check if the file exists
void prep_res_for_file(int file_fd, const char* ext, int status_code_val, char* reason_phrase_val, response* res);
#define prep_res_for_404(file_fd, ext, res) prep_res_for_file(file_fd, ext, 404, "Resource not found.", res)
#define prep_res_for_200(file_fd, ext, res) prep_res_for_file(file_fd, ext, 200, "OK", res)

#define FILE_CALLBACK(callback_name, file_path, status_code_val, reason_phrase_val) \
void callback_name(request* req, response* res) { \
    res->status_line.version = http_version_1p1; \
    res->status_line.status_code = status_code_val; \
    res->status_line.reason_phrase = reason_phrase_val; \
    char* content_type_value = "text/html"; \
    \
    const char *ext = strrchr(file_path, '.'); \
    \
    if (ext && strcmp(ext + 1, "html") == 0) \
    { \
        content_type_value = "text/html"; \
    } else if (ext && strcmp(ext + 1, "css") == 0) \
    { \
        content_type_value = "text/css"; \
    } \
    \
    int file_fd = open(file_path, O_RDONLY); \
    \
    if (file_fd == -1) { \
        res->body.data = NULL; \
        res->body.body_size = 0; \
        return; \
    } \
    \
    struct stat file_stat; \
    fstat(file_fd, &file_stat); \
    \
    res->body.data = cmem_alloc(memory_tag_string, file_stat.st_size + 1); \
    \
    \
    int header_count = 3; \
    header content_type = { \
        .name = "Content-Type", \
        .value = content_type_value, \
    }; \
    header content_length = { \
        .name = "Content-Length", \
        .value = asprintf("%i", file_stat.st_size), \
    }; \
    header connection = { \
        .name = "Connection", \
        .value = "close", \
    }; \
    \
    res->headers.headers = cmem_alloc(memory_tag_response, sizeof(header) * header_count); \
    res->headers.headers[0] = content_type; \
    res->headers.headers[1] = content_length; \
    res->headers.headers[2] = connection; \
    \
    res->headers.header_count = header_count; \
    \
    if (res->body.data) { \
        ssize_t bytes_read = read(file_fd, res->body.data, file_stat.st_size); \
        res->body.data[file_stat.st_size] = '\0'; \
    } \
    \
    res->body.body_size = file_stat.st_size + 1; \
    \
    close(file_fd); \
}