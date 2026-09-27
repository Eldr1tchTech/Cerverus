#include "router.h"

#include "core/containers/darray.h"
#include "core/containers/string.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/util/util.h"
#include "network/http/response.h"
#include "network/network_util.h"
#include "network/routing/route_trie.h"
#include <stddef.h>

router *router_create(router_config *rtr_conf) {
  router *rtr = cmem_alloc(sizeof(router));
  cmem_mcpy(&rtr->conf, rtr_conf, sizeof(router_config));
  rtr->routing_table = trie_create();

  // NOTE: Eventually allow for this to be regenerated more dynamically.
  // for now just generate once (synchronously at runtime)
  string *public_files_darr = _list_files("assets/public");

  rtr->public_directory_hmap =
      hashmap_create(*darray_get_length(public_files_darr), 0.67, 0, nullptr);

  for (size_t i = 0; i < *darray_get_length(public_files_darr); i++) {
    LOG_INFO("find out of bounds read: %s", public_files_darr[i]);
    hashmap_set(rtr->public_directory_hmap, public_files_darr[i], nullptr);
  }

  darray_destroy(public_files_darr);

  return rtr;
}

void router_destroy(router *rtr) {
  hashmap_destroy(rtr->public_directory_hmap);
  trie_destroy(rtr->routing_table);
  cmem_free(rtr);
}

void router_add_route(router *rtr, route *rt) {
  trie_add_route(rtr->routing_table, rt);
}

typedef struct send_file_locals {
  int client_fd;
  string path;
  FILE file;
  response *res;
  string raw_res;
} send_file_locals;

void route_callback_send_file(protothread_state *state) {
  // Just offset calculations, so very cheap
  send_file_locals *locals = (send_file_locals *)state->locals;

  PT_BEGIN(state, route_callback_send_file);
  locals = cmem_realloc(locals, sizeof(send_file_locals));
  locals->res = response_create();

  protothread_state *open_file_state = cmem_alloc(sizeof(protothread_state));
  open_file_state->resume_label = nullptr;
  open_file_state->locals = cmem_alloc(sizeof(open_file_locals));
  ((open_file_locals *)open_file_state->locals)->path = locals->path;
  ((open_file_locals *)open_file_state->locals)->file = &locals->file;
  open_file_state->caller = state;
  PT_WAIT(state, async_io_open_file(open_file_state));

  // Setup status line
  locals->res->status_line.version = http_version_1p1;
  locals->res->status_line.status_code = 200;
  locals->res->status_line.reason_phrase = str_create_lit("OK");

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
  locals->raw_res = response_serialize(locals->res);
  PT_WAIT(state, async_io_send_buffer(locals->raw_res));
  str_destroy(locals->raw_res);

  // Send file
  PT_WAIT(state, async_io_sendfile(locals->file.fd));

  // Cleanup
  cmem_free(locals);
  // NOTE: destroy ctx?

  PT_END(state);
}

typedef struct send_404_locals {
  int client_fd;
  response *res;
  string raw_res;
} send_404_locals;

void route_callback_send_404(protothread_state *state) {
  // Just offset calculations, so very cheap
  send_404_locals *locals = (send_404_locals *)state->locals;

  PT_BEGIN(state, route_callback_send_404);
  locals->res = response_create();

  // Setup status line
  locals->res->status_line.version = http_version_1p1;
  locals->res->status_line.status_code = 404;
  locals->res->status_line.reason_phrase = str_create_lit("Not Found");

  // Setup headers
  // Date

  // Send headers
  locals->raw_res = response_serialize(locals->res); // persistent across await
  PT_WAIT(state, async_io_send_buffer(locals->raw_res));
  str_destroy(locals->raw_res);

  // Cleanup
  cmem_free(locals);
  // NOTE: destroy ctx?

  PT_END(state);
}

void prep_route_callback_send_404(int client_fd) {
  protothread_state *state = cmem_alloc(sizeof(protothread_state));
  state->resume_label = nullptr;
  send_404_locals *locals = cmem_alloc(sizeof(send_404_locals));

  state->locals = locals;
  locals->client_fd = client_fd;

  route_callback_send_404(state);
}

void prep_route_callback_send_file(int client_fd, string path) {
  protothread_state *state = cmem_alloc(sizeof(protothread_state));
  state->resume_label = nullptr;
  send_file_locals *locals = cmem_alloc(sizeof(send_file_locals));

  state->locals = locals;
  locals->client_fd = client_fd;
  locals->path = path;

  route_callback_send_file(state);
}

// TODO: Eventually match to check if given file exists
void router_handle_request(router *rtr, request *request, int client_fd) {
  if (str_equal_lit(request->request_line.URI, "/")) {
    prep_route_callback_send_file(client_fd,
                                  str_create_lit("assets/public/index.html"));
  }

  // 1. Check public directory
  // Implement public directory hashmap here.
  if (request->request_line.method == http_method_get) {
    string temp_file_name = str_dup(request->request_line.URI);
    string ext = str_split_lit(temp_file_name, ".");

    if (ext) {
      if (hashmap_get(rtr->public_directory_hmap, request->request_line.URI)) {
        string path = str_create_lit("assets/public");
        str_cat_str(path, request->request_line.URI);
        prep_route_callback_send_file(client_fd, path);
      }
    }
  }

  // 2. Check against dynamic registered routes
  if (rtr && rtr->routing_table) {
    pt_fn handler =
        trie_find_handler(rtr->routing_table, request->request_line.method,
                          request->request_line.URI);

    protothread_state *state = cmem_alloc(sizeof(protothread_state));
    state->resume_label = nullptr;
    state->locals = cmem_alloc(sizeof(minimal_locals));
    ((minimal_locals *)(state->locals))->client_fd = client_fd;
    ((minimal_locals *)(state->locals))->req = request;

    if (handler) {
      (*handler)(state);
      return;
    }
  }

  // 3. Send 404 if you have made it to this point
}
