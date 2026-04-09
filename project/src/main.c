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

int main()
{
    hashmap* hmap = hashmap_create();
    if (hashmap_entry_exists(hmap, "/user/user.html")) {
        LOG_DEBUG("Exists!");
    } else {
        LOG_DEBUG("Does not exist.");
    }
    // server_config s_conf = {
    //     .port = 8080,
    // };

    // server *s = server_create(&s_conf);

    // server_run(s);
}