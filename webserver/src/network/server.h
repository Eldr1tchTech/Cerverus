#pragma once

#include "core/containers/LRU_cache.h"
#include "core/memory/pool_allocator.h"
#include "network/routing/router.h"

#include <fcntl.h>
#include <liburing.h>
#include <sys/stat.h>

typedef struct server_config {
  int port;
} server_config;

// TODO: complete this
typedef struct server_interface {
  void (*send_file)();
} server_interface;

typedef struct server {
  int socket_fd;
  struct {
    struct io_uring ring;
    pool_allocator *pool_alloc_ctx;
    LRU_cache *LRU_fd_cache;
  } uring;
  server_config *conf;
  router *rtr;
} server;

server *server_create(server_config *s_conf);

void server_run(server *s);

void send_file_response(int client_fd, int file_fd, int status_code,
                        string reason_phrase, string ext);
