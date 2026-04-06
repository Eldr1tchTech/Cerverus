#pragma once

// Router will have a static map of all file paths
// Also the add_route and find_handler methods

#include "network/network_types.inl"

typedef struct router
{
    trie* routing_table;
    darray* public_directory;
} router;

router* router_create();
void router_destroy(router* rtr);

void router_add_route(router* rtr, route* rt);
void router_handle_request(router* rtr, request* req, int client_fd);