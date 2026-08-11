#include "server.h"

#include "network_types.inl"

#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/util/profiler.h"
#include "core/util/util.h"
#include "network/IO/io_uring_helper.h"
#include "network/http/response.h"
#include "network/network_util.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define QUEUE_DEPTH 64

server *server_create(server_config *s_conf) {
  server *s = cmem_alloc(sizeof(server));
  s->conf = s_conf;
  s->rtr = router_create();

  return s;
}

void send_file_response(int client_fd, int file_fd, int status_code,
                        const char *reason_phrase, char *ext) {
  // 1. Assemble response
  // TODO: Eventually use a pool for this
  response *res = response_create(0);

  res->status_line.version = http_version_1p1;
  res->status_line.status_code = status_code;
  res->status_line.reason_phrase = reason_phrase;

  struct stat file_stat;
  fstat(file_fd, &file_stat);

  // Headers
  response_add_header(res, (header){.name = "Content-Type",
                                    .value = content_type_val_helper(ext)});
  char *content_length_str = asprintf_cerv("%i", file_stat.st_size);
  response_add_header(
      res, (header){.name = "Content-Length", .value = content_length_str});
  response_add_header(res, (header){.name = "Connection", .value = "close"});

  // 2. Send response and file
  char *raw = response_serialize(res);
  send(client_fd, raw, strlen(raw), MSG_NOSIGNAL);
  sendfile(client_fd, file_fd, 0, file_stat.st_size);
  cmem_free(raw);
  cmem_free(content_length_str); // Find some way to get rid of this...
  /* IDEA:
  Allocate a buffer that should be big enough, use snprintf, if it fails,
  allocate enough
  */

  close(file_fd);
}

void server_destroy(server *s) { cmem_free(s); }

bool server_setup(server *srv) {

  // Install SIGPIPE handler to prevent crashes
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0; // No SA_RESETHAND — disposition stays ignored permanently
  sigaction(SIGPIPE, &sa, NULL);

  LOG_INFO("Starting server...");
  LOG_INFO("Setting up socket...");

  srv->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (srv->socket_fd == -1) {
    LOG_FATAL("server_start - Socket creation failed.");
    return false;
  }

  int opt = 1;
  if (setsockopt(srv->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    LOG_FATAL("server_start - setsockopt failed.");
    close(srv->socket_fd);
    return false;
  }

  const struct sockaddr_in addr = {.sin_family = AF_INET,
                                   .sin_port = htons(8080),
                                   .sin_addr.s_addr = INADDR_ANY};

  if (bind(srv->socket_fd, &addr, sizeof(addr)) == -1) {
    LOG_FATAL("server_start - Bind failed.");
    close(srv->socket_fd);
    return false;
  }

  socklen_t len = sizeof(addr);
  getsockname(srv->socket_fd, (struct sockaddr *)&addr, &len);

  if (listen(srv->socket_fd, 512) == -1) {
    LOG_FATAL("server_start - Listen failed.");
    close(srv->socket_fd);
    return false;
  }

  LOG_INFO("server_setup - Successful.");
  return true;
}

void server_run(server *srv) {

  if (!server_setup(srv)) {
    return;
  }

  LOG_INFO("Setting up io_uring...");

  s->uring.pool_alloc_ctx = pool_allocator_create(sizeof(uring_context), 64);

  struct io_uring_params params;
  cmem_zmem(&params, sizeof(params));
  params.flags |= IORING_SETUP_SQPOLL | IORING_SETUP_SQ_AFF;
  params.sq_thread_cpu = 3;
  params.sq_thread_idle = 2000; // 2s timeout

  int ret = io_uring_queue_init_params(QUEUE_DEPTH, &s->uring.ring, &params);
  if (ret < 0) {
    LOG_FATAL("server_run - io_uring init failed.");
    close(srv->socket_fd);
    return;
  }

  handle_accept_submission(srv);

  LOG_INFO(
      "Server listening on port %i.\n\tVisit: http://localhost:%i/index.html",
      ntohs(addr.sin_port), ntohs(addr.sin_port));

  while (true) {
    uring_process_completions(srv);
  }

  close(srv->socket_fd);
  io_uring_queue_exit(&srv->uring.ring);

  server_destroy(srv);
}
