#pragma once

// Router will have a static map of all file paths
// Also the add_route and find_handler methods

#include "core/containers/hashmap.h"
#include "network/network_types.inl"

typedef struct router_config {
} router_config;

typedef struct router_t {
  router_config conf;
  trie *routing_table;
  hashmap *public_directory_hmap;
} router_t;

router_t *router_create(router_config *rtr_conf);
void router_destroy(router_t *rtr);

void router_add_route(router_t *rtr, route_t *rt);
void router_handle_request(router_t *rtr, request_t *req, int client_fd);