#include "router.h"

#include "core/memory/cmem.h"
#include "core/util/util.h"
#include "network/routing/route_trie.h"

#include <string.h>

router* router_create() {
    router* rtr = cmem_alloc(memory_tag_router, sizeof(router));

    return rtr;
}

void router_destroy(router* rtr) {
    cmem_free(memory_tag_router, rtr);
}

void router_add_route(router* rtr, route* rt) {
    trie_add_route(rtr->routing_table, rt);
}

// TODO: Eventually match to check if given file exists
void router_handle_request(router* rtr, request* request, int client_fd) {
    if (strcmp(request->request_line.URI, "/") == 0)
    {
        int file_fd = open("assets/public/index.html", O_RDONLY);
        if (file_fd != -1)
        {
            send_file_response(client_fd, file_fd, 200, "OK", ".html");
            return;
        }
    }

    // 1. Check public directory
    if (request->request_line.method == http_method_get)
    {
        const char *ext = strrchr(request->request_line.URI, '.');
        if (ext)
        {
            char* file_name = asprintf("assets/public%s", request->request_line.URI);
            int file_fd = open(file_name, O_RDONLY);
            if (file_fd != -1)
            {
                send_file_response(client_fd, file_fd, 200, "OK", ext);
                cmem_free(memory_tag_string, file_name);
                return;
            }
            cmem_free(memory_tag_string, file_name);
        }
    }

    // 2. Check against dynamic registered routes
    route_callback handler = trie_find_handler(s->route_trie, request->request_line.method, request->request_line.URI);
    if (handler)
    {
        (*handler)(request, client_fd);
        return;
    }

    // 3. Send 404 if you have made it to this point
    int file_fd = open("assets/404.html", O_RDONLY);
    if (file_fd != -1)
    {
        send_file_response(client_fd, file_fd, 404, "Not Found", ".html");
    }
}