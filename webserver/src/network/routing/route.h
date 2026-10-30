#pragma once

#include "network/network_types.inl"

route_t *route_create(http_method method, sds URI, pt_fn callback);
void route_destroy(route_t *rt);