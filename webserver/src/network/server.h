#pragma once

#include "network/router.h"
#include "network/command_buffer.h"

typedef struct server_config
{
    int port;
} server_config;

typedef struct server
{
    int port;
    int socket_fd;
    command_buffer* cmd_buff;
    router* r;
} server;

server* server_create(server_config s_conf);

void server_run(server* s);