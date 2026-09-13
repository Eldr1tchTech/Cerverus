#include "server.h"

#include "network_types.inl"

#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/util/profiler.h"
#include "network/IO/async_io.h"
#include "network/http/response.h"
#include "network/network_util.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

server *server_create(server_config *s_conf, router *rtr) {
  server *s = cmem_alloc(sizeof(server));
  s->conf = s_conf;
  s->rtr = rtr;

  return s;
}

void server_destroy(server *s) { cmem_free(s); }

bool server_setup(server *srv) {

  // Install SIGPIPE handler to prevent crashes
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0; // No SA_RESETHAND — disposition stays ignored permanently
  sigaction(SIGPIPE, &sa, nullptr);

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

  // TODO: Figure out how accepts will be handles, most likely currently by the
  // async ctx?

  LOG_INFO(
      "Server listening on port %i.\n\tVisit: http://localhost:%i/index.html",
      srv->conf->port, srv->conf->port);

  while (true) {
    async_io_process();
  }

  close(srv->socket_fd);

  server_destroy(srv);
}