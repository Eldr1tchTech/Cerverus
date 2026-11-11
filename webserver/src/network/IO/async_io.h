#pragma once

#include "core/containers/sv.h"
#include "core/util/protothread.h"
#include "defines.h"
#include <linux/stat.h>
#include <sys/socket.h>

// ==== LIFETIME ================
/*
- init, shutdown, tick
*/

// LRU cache, registered fd table, ring_helper_init
bool async_io_init();
void async_io_shutdown();

// Just serve as a wrapper to ring_helper_tick
void async_io_tick();

// ==== FILE HANDLING ===========
/*
- realease and acquire functions to count refs
- openat, statx, and close interface with the OS/uring
*/
typedef struct file_t {
  int fd;
  size_t refs;
  sds name;
  sds path;
  struct statx statx_buff;
} file_t;

typedef struct async_open_file_args_t {
  sv_t path;
  file_t *f;
} async_open_file_args_t;

// Checks LRU cache handles the lifetime modification section.
void async_io_open_file(protothread_state *pt);

// Checks whether it can be closed, handles all that jazz
void async_io_close_file(file_t *f);

// ==== NETWORKING ==============
/*
- Setting up state (server socket)
- API
  - (recv) some way to hook into the receive so that what is present can be
validated on whether it's ready to be processed or if it's unfinished, pass a
function pointer so that it can be extended to be used for other purposes such
as chunked transfer-encoding (uploading/downloading files)
  - (send/send_all)
  - (sendfile [-> pipe2, splice/splice_all]) figure out pipelining for these
*/

typedef struct async_socket_args_t {
  int domain;
  int type;
  int protocol;
  unsigned int flags;
  int sock;
} async_socket_args_t;

void async_io_socket(protothread_state *pt);

typedef struct async_multishot_accept_args_t {
  int fd;
  struct sockaddr *addr;
  socklen_t* addrlen;
  int flags;
} async_multishot_accept_args_t;

// do I even take a pt for this? probably not right???
void async_io_multishot_accept(protothread_state *pt);

// @returns nullptr if unfinished, otherwise the output parsed into expected
// struct.
typedef void *(*async_recv_args_validation_fn)(sds buffer);

typedef struct async_recv_args_t {
  int client_fd;
  sds buffer;
  async_recv_args_validation_fn validation_fn;
} async_recv_args_t;

void async_io_recv(protothread_state *pt);

typedef struct async_send_buffer_args_t {
  int client_fd;
  sds buffer;
} async_send_buffer_args_t;

void async_io_send_buffer(protothread_state *pt);

typedef struct async_sendfile_args_t {
  int client_fd;
  int file_fd;
} async_sendfile_args_t;

void async_io_sendfile(protothread_state *pt);