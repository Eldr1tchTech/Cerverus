#include "route.h"

#include "core/memory/cmem.h"
#include "core/util/util.h"
#include "network/network_util.h"

route *route_create(http_method method, char *URI, route_callback callback)
{
    route *new_route = cmem_alloc(memory_tag_route, sizeof(route));

    new_route->segments = parse_URI(URI);

    new_route->method = method;
    new_route->callback = callback;

    return new_route;
}

void route_destroy(route *rt)
{
    route_segment* darr_data = rt->segments->data;
    for (int i = 0; i < rt->segments->length; i++)
    {
        cmem_free(memory_tag_string, darr_data[i].path_segment);
    }
    darray_destroy(rt->segments);
    cmem_free(memory_tag_route, rt);
}