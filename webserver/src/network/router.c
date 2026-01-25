#include "router.h"

#include "core/util/logger.h"
#include "core/memory/cmem.h"
#include "core/util/profiler.h"
#include "network/response.h"
#include "core/util/util.h"

#include <string.h>

router *router_create(router_config r_conf)
{
    router *r = cmem_alloc(memory_tag_router, sizeof(router));

    r->r_conf = r_conf;
    r->routes = darray_create(8, sizeof(route));

    return r;
}

// This is a bad way to do it if sendfile is to be used.
void router_handle_route(router *r, request *req, response *res)
{
    if (!r)
    {
        LOG_ERROR("router_handle_route - Please provide a valid router argument.");
        return;
    }

    // 1. Check public directory
    if (r->r_conf.use_public_directory)
    {
        if (req->request_line.method == http_method_get)
        {
            const char *ext = strrchr(req->request_line.URI, '.');
            if (ext)
            {
                int file_fd = open(asprintf("assets/public%s", req->request_line.URI), O_RDONLY);
                if (file_fd != -1)
                {
                    prep_res_for_200(file_fd, ext, res);
                }
            }
        }

        // 2. Check custom routes
        route *routes_data = (route *)r->routes->data;
        for (int i = 0; i < r->routes->length; i++)
        {
            LOG_DEBUG("attempting to match route");
            route rt = routes_data[i];
            if (rt.method == req->request_line.method)
            {
                LOG_DEBUG("method matches.");
                if (strcmp(rt.URI, req->request_line.URI) == 0)
                {
                    LOG_DEBUG("URI matches.");

                    profile_operation("callback", rt.callback(req, res));

                    return;
                }
            }
        }

        r->r_conf.default_route(req, res);
    }
}

void router_destroy(router *r)
{
    darray_destroy(r->routes);
    cmem_free(memory_tag_router, r);
    r = 0;
}

void prep_res_for_file(int file_fd, const char *ext, int status_code_val, char *reason_phrase_val, response *res)
{
    res->status_line.version = http_version_1p1;
    res->status_line.status_code = status_code_val;
    res->status_line.reason_phrase = reason_phrase_val;

    struct stat file_stat;
    fstat(file_fd, &file_stat);

    res->body.data = cmem_alloc(memory_tag_string, file_stat.st_size + 1);

    int header_count = 3;
    header content_type = {
        .name = "Content-Type",
        .value = content_type_val_helper(ext),
    };
    header content_length = {
        .name = "Content-Length",
        .value = asprintf("%i", file_stat.st_size),
    };
    header connection = {
        .name = "Connection",
        .value = "close",
    };

    res->headers.headers = cmem_alloc(memory_tag_response, sizeof(header) * header_count);
    res->headers.headers[0] = content_type;
    res->headers.headers[1] = content_length;
    res->headers.headers[2] = connection;

    res->headers.header_count = header_count;

    if (res->body.data)
    {
        ssize_t bytes_read = read(file_fd, res->body.data, file_stat.st_size);
        res->body.data[file_stat.st_size] = '\0';
    }

    res->body.body_size = file_stat.st_size + 1;

    close(file_fd);
    return;
}