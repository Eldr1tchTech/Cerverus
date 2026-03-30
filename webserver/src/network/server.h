#pragma once

#include "network/network_types.inl"

#include "core/containers/darray.h"
#include "network/route_trie.h"

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
} server;

server* server_create(server_config* s_conf);

void server_add_route(server* s, route* rt);
void server_handle_request(server* s, request* req, int client_fd);

void server_run(server* s);

void send_file_response(int client_fd, int file_fd, int status_code, const char *reason_phrase, char *ext);