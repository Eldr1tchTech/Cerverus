#include <network/request.h>
#include <network/server.h>
#include <network/response.h>
#include <core/memory/cmem.h>
#include <core/util/logger.h>
#include <core/util/util.h>
#include <network/route.h>

#include <string.h>
#include <sys/socket.h>
#include <math.h>
#include <fcntl.h>

int main()
{
    server_config s_conf = {
        .io_uring_queue_depth = 32,
        .port = 8080,
        .keep_alive = {
            .duration = 16,
            .queue_depth = 32,
        },
    };

    server *s = server_create(&s_conf);

    server_run(s);
}