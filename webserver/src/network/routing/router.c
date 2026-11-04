#include "router.h"

#include "core/containers/darray.h"

#include "core/containers/sv.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/util/protothread.h"
#include "core/util/util.h"
#include "core/vendor/sds.h"
#include "defines.h"
#include "network/IO/async_io.h"
#include "network/http/response.h"
#include "network/network_util.h"
#include "network/routing/route_trie.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

router_t *router_create(router_config *rtr_conf) {
  router_t *rtr = cmem_alloc(sizeof(router_t));
  cmem_mcpy(&rtr->conf, rtr_conf, sizeof(router_config));
  rtr->routing_table = trie_create();

  // NOTE: Eventually allow for this to be regenerated more dynamically.
  // for now just generate once (synchronously at runtime)
  sds *public_files_darr = _list_files("assets/public");

  rtr->public_directory_hmap =
      hashmap_create(*darray_get_length(public_files_darr), 0.67, 0, nullptr);

  for (size_t i = 0; i < *darray_get_length(public_files_darr); i++) {
    hashmap_set(rtr->public_directory_hmap,
                (sv_t){.ptr = public_files_darr[i],
                       .len = sdslen(public_files_darr[i])},
                nullptr);
  }

  darray_destroy(public_files_darr);

  return rtr;
}

void router_destroy(router_t *rtr) {
  hashmap_destroy(rtr->public_directory_hmap);
  trie_destroy(rtr->routing_table);
  cmem_free(rtr);
}

void router_add_route(router_t *rtr, route_t *rt) {
  trie_add_route(rtr->routing_table, rt);
}

typedef struct send_file_locals {
  int client_fd;
  sds path;
  file_t file;
  response_t *res;
  sds raw_res;
  protothread_state *open_file_state;
  protothread_state *send_buffer_state;
  protothread_state *sendfile_state;
} send_file_locals;

void route_callback_send_file(protothread_state *state) {
  // Just offset calculations, so very cheap
  send_file_locals *locals = (send_file_locals *)state->locals;

  PT_BEGIN(state, route_callback_send_file);
  locals = cmem_realloc(locals, sizeof(send_file_locals));
  locals->res = response_create();

  locals->open_file_state =
      cmem_new(protothread_state, .resume_label = nullptr, .caller = state,
               .locals = cmem_new(open_file_locals, .path = locals->path,
                                  .file = &locals->file));
  PT_WAIT(state, async_io_open_file(locals->open_file_state));
  cmem_free(locals->open_file_state->locals);
  cmem_free(locals->open_file_state);

  // Setup status line
  locals->res->status_line.version = http_version_1p1;
  locals->res->status_line.status_code = 200;
  locals->res->status_line.reason_phrase = sdsnewlit("OK");

  // Setup headers

  // TODO: Make this more user-friendly
  // Content-Type
  char *dot = memchr(locals->file.name, '.', sdslen(locals->file.name));
  if (!dot)
    LOG_ERROR("no extention present.");
  sv_t type = {.ptr = dot + 1,
               .len =
                   locals->file.name + sdslen(locals->file.name) - (dot + 1)};
  header_t h = {.sds = {.name = sdsnewlit("Content-Type"),
                        .value = sdsnew(content_type_val_helper(type))}};
  response_add_header(locals->res, h);

  // Content-Length
  h.sds.name = sdsnewlit("Content-Length");
  h.sds.value = sdsfromlonglong(locals->file.statx_buff.stx_size);
  response_add_header(locals->res, h);

  // Date

  // Send headers
  locals->raw_res = response_serialize(locals->res);
  locals->send_buffer_state =
      cmem_new(protothread_state, .resume_label = nullptr, .caller = state,
               .locals = cmem_new(send_buffer_locals, .buffer = locals->raw_res,
                                  .client_fd = locals->client_fd));
  PT_WAIT(state, async_io_send_buffer(locals->send_buffer_state));
  cmem_free(locals->send_buffer_state->locals);
  cmem_free(locals->send_buffer_state);
  sdsfree(locals->raw_res);

  // Sendfile
  locals->sendfile_state = cmem_new(
      protothread_state, .resume_label = nullptr, .caller = state,
      .locals = cmem_new(sendfile_locals, .client_fd = locals->client_fd,
                         .file = &locals->file));
  PT_WAIT(state, async_io_sendfile(locals->sendfile_state));
  cmem_free(locals->sendfile_state->locals);
  cmem_free(locals->sendfile_state);

  PT_WAIT(state, handle_close_submission(locals->client_fd, state));

  // Cleanup
  cmem_free(locals);
  // NOTE: destroy ctx?

  PT_END(state);
}

typedef struct send_404_locals {
  int client_fd;
  response_t *res;
  sds raw_res;
} send_404_locals;

void route_callback_send_404(protothread_state *state) {
  // Just offset calculations, so very cheap
  send_404_locals *locals = (send_404_locals *)state->locals;

  PT_BEGIN(state, route_callback_send_404);
  locals->res = response_create();

  // Setup status line
  locals->res->status_line.version = http_version_1p1;
  locals->res->status_line.status_code = 404;
  locals->res->status_line.reason_phrase = sdsnewlit("Not Found");

  // Setup headers
  // Date

  // Send headers
  locals->raw_res = response_serialize(locals->res);
  protothread_state *send_buffer_state =
      cmem_new(protothread_state, .resume_label = nullptr, .caller = state,
               .locals = cmem_new(send_buffer_locals, .buffer = locals->raw_res,
                                  .client_fd = locals->client_fd));
  PT_WAIT(state, async_io_send_buffer(send_buffer_state));
  cmem_free(send_buffer_state->locals);
  cmem_free(send_buffer_state);
  sdsfree(locals->raw_res);

  PT_WAIT(state, handle_close_submission(locals->client_fd, state));

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

void prep_route_callback_send_file(int client_fd, sds path) {
  protothread_state *state = cmem_alloc(sizeof(protothread_state));
  state->resume_label = nullptr;
  send_file_locals *locals = cmem_alloc(sizeof(send_file_locals));

  state->locals = locals;
  locals->client_fd = client_fd;
  locals->path = path;

  route_callback_send_file(state);
}

// TODO: Eventually match to check if given file exists
void router_handle_request(router_t *rtr, request_t *request, int client_fd) {
  if (sv_eq_lit(request->status_line.URI, "/")) {
    prep_route_callback_send_file(client_fd,
                                  sdsnewlit("assets/public/index.html"));
  }

  // 1. Check public directory
  if (request->status_line.method == http_method_get) {
    char *ext =
        memchr(request->status_line.URI.ptr, '.', request->status_line.URI.len);
    if (ext) {
      if (hashmap_get(rtr->public_directory_hmap, request->status_line.URI)) {
        sds path = sdsnewlit("assets/public");
        path = sdscatlen(path, request->status_line.URI.ptr,
                         request->status_line.URI.len);
        prep_route_callback_send_file(client_fd, path);
      }
    }
  }

  // 2. Check against dynamic registered routes
  if (rtr && rtr->routing_table) {
    sds URI_sds = svtosds(request->status_line.URI);
    pt_fn handler = trie_find_handler(rtr->routing_table,
                                      request->status_line.method, URI_sds);

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
