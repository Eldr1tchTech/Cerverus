#pragma once

#include "network/network_types.inl"

route* route_create(http_method method, char *URI, route_callback callback);
void route_destroy(route* rt);