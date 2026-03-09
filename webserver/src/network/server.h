#pragma once

#include "core/containers/darray.h"
#include "network/network_types.inl"

typedef void (*route_callback)(request* req, int client_fd);

typedef struct route
{
    http_method method;
    char* URI;
    route_callback callback;
} route;

typedef struct server
{
    int socket_fd;
    // TODO: Change to use an trie
    darray* routes;
} server;

server* server_create();

void server_add_route(server* s, route* rt);
void server_handle_request(server* s, request* req, int client_fd);

void server_run(server* s);