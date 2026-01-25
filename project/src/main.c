#include <network/request.h>
#include <network/server.h>
#include <network/router.h>
#include <core/memory/cmem.h>
#include <core/util/logger.h>
#include <core/util/util.h>

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

void handle_sigpipe(int sig) {
    // Ignore SIGPIPE to prevent crashes when client disconnects
    fprintf(stderr, "Caught SIGPIPE, client disconnected unexpectedly\n");
}

FILE_CALLBACK(default_callback, "assets/public/404.html", 404, "Not Found");
FILE_CALLBACK(index_callback, "assets/public/index.html", 200, "OK");

int main() {
    server_config s_conf = {
        .port = 8080,
    };

    server* s = server_create(s_conf);

    router_config r_conf = {
        .default_route = default_callback,
        .use_public_directory = true,
    };

    s->r = router_create(r_conf);

    route slash_route = {
        .method = http_method_get,
        .URI = "/",
        .callback = index_callback,
    };

    darray_add(s->r->routes, &slash_route);

    server_run(s);
}