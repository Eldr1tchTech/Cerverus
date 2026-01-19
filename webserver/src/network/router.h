#pragma once

#include "core/containers/darray.h"
#include "network_types.inl"

// TODO: Change this to use a command buffer of some sort for the server?
typedef void (*route_callback)(request* req, int client_fd);

typedef struct route
{
    http_method method;
    char* URI;
    route_callback callback;
} route;

typedef struct router
{
    darray* routes;
} router;
