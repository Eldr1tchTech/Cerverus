#pragma once

#include "network/IO/async_io.h"
#include "network/network_types.inl"

route *route_create(http_method method, char *URI, async_resume_fn callback);
void route_destroy(route *rt);