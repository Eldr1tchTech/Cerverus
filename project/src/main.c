#include <network/request.h>
#include <network/server.h>
#include <network/router.h>
#include <core/memory/cmem.h>
#include <core/util/logger.h>
#include <core/util/util.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

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

void handle_sigpipe(int sig) {
    // Ignore SIGPIPE to prevent crashes when client disconnects
    fprintf(stderr, "Caught SIGPIPE, client disconnected unexpectedly\n");
}

FILE_CALLBACK(index_callback, "assets/index.html", 200, "OK");
FILE_CALLBACK(architecture_callback, "assets/architecture.html", 200, "OK");
FILE_CALLBACK(features_callback, "assets/features.html", 200, "OK");
FILE_CALLBACK(default_callback, "assets/404.html", 404, "OK");
FILE_CALLBACK(style_callback, "assets/style.css", 200, "OK");

int main() {
    server_config s_conf = {
        .port = 8080,
    };

    server* s = server_create(s_conf);

    s->r = router_create(default_callback);

    route slash_route = {
        .method = http_method_get,
        .URI = "/",
        .callback = index_callback,
    };
    route index_route = {
        .method = http_method_get,
        .URI = "/index.html",
        .callback = index_callback,
    };
    route architecture_route = {
        .method = http_method_get,
        .URI = "/architecture.html",
        .callback = architecture_callback,
    };
    route features_route = {
        .method = http_method_get,
        .URI = "/features.html",
        .callback = features_callback,
    };
    route style_route = {
        .method = http_method_get,
        .URI = "/style.css",
        .callback = style_callback,
    };

    darray_add(s->r->routes, &slash_route);
    darray_add(s->r->routes, &index_route);
    darray_add(s->r->routes, &architecture_route);
    darray_add(s->r->routes, &features_route);
    darray_add(s->r->routes, &style_route);

    server_run(s);
}