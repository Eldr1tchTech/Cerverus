#include "router.h"

#include "core/memory/cmem.h"
#include "network/http/response.h"
#include "network/network_util.h"
#include "network/routing/route_trie.h"

router *router_create(router_config *rtr_conf) {
  router *rtr = cmem_alloc(sizeof(router));
  cmem_mcpy(&rtr->conf, rtr_conf, sizeof(router_config));
  rtr->routing_table = trie_create();

  return rtr;
}

void router_destroy(router *rtr) {
  cmem_free(rtr);
  trie_destroy(rtr->routing_table);
}

void router_add_route(router *rtr, route *rt) {
  trie_add_route(rtr->routing_table, rt);
}

typedef struct send_file_locals {
  int client_fd;
  string path;
  FILE file;
  response *res;
} send_file_locals;

void route_callback_send_file(protothread_state *state) {
  // Just offset calculations, so very cheap
  send_file_locals *locals = (send_file_locals *)state->locals;

  PT_BEGIN(state, route_callback_send_file);
  locals = cmem_realloc(state->locals, sizeof(send_file_locals));

  protothread_state *open_file_state = cmem_alloc(sizeof(protothread_state));
  open_file_state->locals = cmem_alloc(sizeof(open_file_locals));
  ((open_file_locals *)open_file_state->locals)->path = locals->path;
  ((open_file_locals *)open_file_state->locals)->file = &locals->file;
  open_file_state->caller = state;
  PT_WAIT(state, async_io_open_file(open_file_state));

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
  string raw_res = response_serialize(locals->res); // persistent across await
  PT_WAIT(state, async_io_send_buffer(raw_res));
  str_destroy(raw_res);

  // Send file
  PT_WAIT(state, async_io_sendfile(locals->file.fd));

  // Cleanup
  cmem_free(locals);
  // NOTE: destroy ctx?

  PT_END(state);
}

void prep_route_callback_send_file(int client_fd, string path);

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
      char *file_name = "assets/public";
      str_cat_str(file_name, request->request_line.URI);
      int file_fd = open(file_name, O_RDONLY);
      if (file_fd != -1) {
        route_callback_send_file(client_fd, file_fd, 200, "OK", ext);
        cmem_free(file_name);
        return;
      }
      cmem_free(file_name);
    }
  }

  // 2. Check against dynamic registered routes
  if (rtr && rtr->routing_table) {
    pt_fn handler =
        trie_find_handler(rtr->routing_table, request->request_line.method,
                          request->request_line.URI);

    protothread_state *state = cmem_alloc(sizeof(protothread_state));
    state->locals = cmem_alloc(sizeof(minimal_locals));
    ((minimal_locals *)(state->locals))->client_fd = client_fd;
    ((minimal_locals *)(state->locals))->req = request;

    if (handler) {
      (*handler)(state);
      return;
    }
  }

  // 3. Send 404 if you have made it to this point
  int file_fd = open("assets/404.html", O_RDONLY);
  if (file_fd != -1) {
    route_callback_send_file(client_fd, file_fd, 404, "Not Found", ".html");
  }
}
