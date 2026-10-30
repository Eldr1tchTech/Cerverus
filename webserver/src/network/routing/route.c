#include "route.h"

#include "core/memory/cmem.h"
#include "core/util/util.h"
#include "core/vendor/sds.h"

route_t *route_create(http_method method, sds URI, pt_fn callback) {
  route_t *new_route = cmem_alloc(sizeof(route_t));

  int count = 0;
  sds *strs = sdssplitlen(URI, sdslen(URI), "/", 1, &count);

  new_route->segments_darr =
      darray_create(count, sizeof(route_segment_t));

  route_segment_t temp_segment;
  for (int i = 0; i < count; i++) {
    temp_segment.path_segment = strs[i];
    temp_segment.is_dynamic = (strs[i][0] == ':') ? true : false;
    new_route->segments_darr =
        darray_add(new_route->segments_darr, &temp_segment);
  }
  new_route->method = method;
  new_route->callback = callback;

  return new_route;
}

void route_destroy(route_t *rt) {
  darray_destroy_string_helper(rt->segments_darr);
  darray_destroy(rt->segments_darr);
  cmem_free(rt);
}
