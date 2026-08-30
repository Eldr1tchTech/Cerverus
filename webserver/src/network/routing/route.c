#include "route.h"

#include "core/memory/cmem.h"
#include "core/util/util.h"
#include "network/network_util.h"

route *route_create(http_method method, char *URI, async_resume_fn callback) {
  route *new_route = cmem_alloc(sizeof(route));

  new_route->segments = parse_URI(URI);

  new_route->method = method;
  new_route->callback = callback;

  return new_route;
}

void route_destroy(route *rt) {
  darray_destroy_string_helper(rt->segments);
  darray_destroy(rt->segments);
  cmem_free(rt);
}
