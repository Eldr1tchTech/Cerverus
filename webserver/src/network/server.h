#pragma once

#include "network/network_types.inl"

#include "core/containers/darray.h"
#include "network/routing/route_trie.h"
#include "network/routing/router.h"

#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <liburing.h>

typedef struct server_config
{
    int port;
} server_config;

typedef struct server
{
    int socket_fd;
    trie* route_trie;
    struct io_uring ring;
    server_config* conf;
    router* rtr;
} server;

server* server_create(server_config* s_conf);

void server_run(server* s);

void send_file_response(int client_fd, int file_fd, int status_code, const char *reason_phrase, char *ext);