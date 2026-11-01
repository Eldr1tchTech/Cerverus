
#include "core/containers/sv.h"
#include "core/util/protothread.h"
#include "core/vendor/sds.h"
#include "defines.h"
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
#include <string.h>
#include <sys/socket.h>

#include <core/containers/doubly_linked_list.h>
#include <core/containers/hashmap.h>

// TODO: create logical_test_locals? or some way for the server to pass the
// appropriate information on to the handler
typedef struct test_locals {
  int client_fd;
  request_t *req;
  file_t file;
  response_t *res;
} test_locals;

void route_callback_test(protothread_state *state) {
  // Just offset calculations, so very cheap
  test_locals *locals = (test_locals *)state->locals;

  PT_BEGIN(state, route_callback_test);
  locals = cmem_realloc(state->locals, sizeof(test_locals));
  locals->res = response_create();

  protothread_state *open_file_state = cmem_alloc(sizeof(protothread_state));
  open_file_state->resume_label = nullptr;
  open_file_state->locals = cmem_alloc(sizeof(open_file_locals));
  ((open_file_locals *)open_file_state->locals)->path =
      sdsnewlit("assets/public/test.html");
  ((open_file_locals *)open_file_state->locals)->file = &locals->file;
  open_file_state->caller = state;
  PT_WAIT(state, async_io_open_file(open_file_state));

  // Setup status line
  locals->res->status_line.version = http_version_1p1;
  locals->res->status_line.status_code = 200;
  locals->res->status_line.reason_phrase = sdsnewlit("OK");

  // Setup headers

  // TODO: Make this more user-friendly
  // Content-Type
  char *ext = memchr(locals->file.name, '.', sdslen(locals->file.name));
  header_t h = {
      .sds = {.name = sdsnewlit("Content-Type"),
              .value = sdsnew(content_type_val_helper((sv_t){
                  .ptr = ext + 1, .len = ext - locals->file.name - 1}))}};
  response_add_header(locals->res, h);

  // Content-Length
  h.sds.name = sdsnewlit("Content-Length");
  h.sds.value = sdsfromlonglong(locals->file.statx_buff.stx_size);
  response_add_header(locals->res, h);

  // Date

  // Send headers
  sds raw_res = response_serialize(locals->res); // persistent across await
  PT_WAIT(state, async_io_send_buffer(raw_res));
  sdsfree(raw_res);

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
  router_t *rtr = router_create(&rtr_conf);

  sds temp_str = sdsnewlit("/test");
  route_t *rt_test = route_create(http_method_get, temp_str, route_callback_test);

  router_add_route(rtr, rt_test);

  // Server setup
  server_config srv_conf = {
      .port = 8080,
  };
  server *srv = server_create(&srv_conf, rtr);

  server_setup(srv);

  async_io_update_values(srv->socket_fd, rtr);

  server_run(srv);

  sdsfree(temp_str);
  async_io_shutdown();
}
