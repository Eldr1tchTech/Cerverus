#pragma once

#include "network/IO/async_io.h"
#include "network/network_types.inl"

route *route_create(http_method method, string URI, pt_fn callback);
void route_destroy(route *rt);