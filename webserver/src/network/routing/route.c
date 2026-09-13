#include "route.h"

#include "core/memory/cmem.h"
#include "core/util/util.h"
#include "network/network_util.h"

route *route_create(http_method method, char *URI, pt_fn callback) {
  route *new_route = cmem_alloc(sizeof(route));

  string *str_darr = parse_URI(URI);

  new_route->segments_darr =
      darray_create(*darray_get_length(str_darr), sizeof(route_segment));

  for (int i = 0; i < *darray_get_length(str_darr); i++) {
    route_segment temp_segment = {.path_segment = str_darr[i],
                                  .is_dynamic =
                                      (str_darr[i][0] == ':') ? true : false};
    darray_add(new_route->segments_darr, &temp_segment);
  }
  darray_destroy(str_darr);

  new_route->method = method;
  new_route->callback = callback;

  return new_route;
}

void route_destroy(route *rt) {
  darray_destroy_string_helper(rt->segments_darr);
  darray_destroy(rt->segments_darr);
  cmem_free(rt);
}
