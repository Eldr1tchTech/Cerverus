#include "core/util/protothread.h"
#include "network/IO/async_io.h"
#include "network/network_util.h"
#include <core/memory/cmem.h>
#include <core/util/logger.h>
#include <core/util/util.h>
#include <network/http/request.h>
#include <network/http/response.h>
#include <network/routing/route.h>
#include <network/server.h>

#include <fcntl.h>
#include <sys/socket.h>

#include <core/containers/doubly_linked_list.h>
#include <core/containers/hashmap.h>

// TODO: create logical_test_locals? or some way for the server to pass the
// appropriate information on to the handler
typedef struct test_locals {
  request *req;
  int client_fd;
  FILE file;
  response *res;
} test_locals;

void route_callback_test(void *ctx) {
  // Just offset calculations, so very cheap
  protothread_state *state = ctx;
  test_locals *locals = ((logical_async_context *)ctx)->local;

  PT_BEGIN(state, route_callback_test);
  locals = cmem_alloc(sizeof(test_locals));

  PT_WAIT(state, async_io_open_file("assets/public/test.html", &locals->file));

  // Setup status line
  locals->res->status_line.version = http_version_1p1;
  locals->res->status_line.status_code = 200;
  locals->res->status_line.reason_phrase = "OK";

  // Setup headers

  // TODO: Make this more user-friendly
  // Content-Type
  string str_temp = str_dup(locals->file.name);
  string str_type = str_split_lit(str_temp, ".");
  header h = {.name = str_create_lit("Content-Type"),
              .value = content_type_val_helper(str_type)};
  response_add_header(locals->res, h);
  str_destroy(str_type);
  str_destroy(str_temp);

  // Content-Length
  h.name = str_create_lit("Content-Length");
  h.value = str_empty();
  str_cat_u64(h.value, locals->file.statx_buff.stx_size);
  response_add_header(locals->res, h);

  // Date

  // Send headers
  string raw_res = response_serialize(locals->res);
  PT_WAIT(state, async_io_send_buffer(raw_res));
  str_destroy(raw_res);

  // Send file
  PT_WAIT(state, async_io_sendfile(locals->file.fd));

  // Cleanup
  cmem_free(locals);
  // NOTE: destroy ctx?

  PT_END(state);
}

int main() {
  async_io_setup();

  // Router setup
  router_config rtr_conf = {};
  router *rtr = router_create(&rtr_conf);

  route *rt_test = route_create(http_method_get, "/test", route_callback_test);

  router_add_route(rtr, rt_test);

  // Server setup
  server_config srv_conf = {
      .port = 8080,
  };
  server *srv = server_create(&srv_conf, rtr);

  server_run(srv);

  async_io_shutdown();
}
