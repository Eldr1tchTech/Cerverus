#include <network/http/request.h>
#include <network/server.h>
#include <network/http/response.h>
#include <core/memory/cmem.h>
#include <core/util/logger.h>
#include <core/util/util.h>
#include <network/routing/route.h>

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <math.h>
#include <fcntl.h>

#include <core/containers/hashmap.h>
#include <core/containers/doubly_linked_list.h>

int main()
{
    server_config srv_conf = {
        .port = 8080,
    };

    server *srv = server_create(&srv_conf);

    server_run(srv);
}