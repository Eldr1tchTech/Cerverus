#include "router.h"

// TODO: rewrite the send_file_response using protothreads/async properly, also
// remember to properly use the caller resume in all of the uring methods that
// take a protothread_state

#include "core/memory/cmem.h"
#include "network/routing/route_trie.h"

router *router_create(router_config *rtr_conf) {
  router *rtr = cmem_alloc(sizeof(router));
  cmem_mcpy(&rtr->conf, rtr_conf, sizeof(router_config));

  return rtr;
}

void router_destroy(router *rtr) { cmem_free(rtr); }

void router_add_route(router *rtr, route *rt) {
  trie_add_route(rtr->routing_table, rt);
}

// TODO: Eventually match to check if given file exists
void router_handle_request(router *rtr, request *request, int client_fd) {
  if (str_equal_lit(request->request_line.URI, "/")) {
    int file_fd = open("assets/public/index.html", O_RDONLY);
    if (file_fd != -1) {
      send_file_response(client_fd, file_fd, 200, "OK", ".html");
      return;
    }
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
        send_file_response(client_fd, file_fd, 200, "OK", ext);
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
    send_file_response(client_fd, file_fd, 404, "Not Found", ".html");
  }
}
