#pragma once

#include <liburing.h>
#include <stddef.h>
#include <stdint.h>

#include "core/util/protothread.h"
#include "network/network_types.inl"
#include "network/routing/router.h"

typedef enum uring_op_type {
  uring_op_type_accept,
  uring_op_type_recv,
  uring_op_type_openat,
  uring_op_type_send,
  uring_op_type_close,
  uring_op_type_statx,
  uring_op_type_splice,
  uring_op_type_splice_all,
} uring_op_type;

typedef struct file_t {
  int fd;
  sds name;
  sds path;
  struct statx statx_buff;
} file_t;

// TODO: Make this internal and get rid of the seperate struct declaration.
typedef struct recv_context {
  int client_fd;
  sds buffer;
  size_t offset;
  request_t request;
} recv_context;

typedef struct splice_all_context {
  int fd_in, fd_out;
  uint64_t off_in;
  int pipe[2];
  unsigned int nbytes;
  // Internal
  size_t in_pipe;
  bool is_draining;
} splice_all_context;

typedef struct logical_async_context {
  uring_op_type op_type;
  protothread_state *pt_state;
  union {
    recv_context recv;
    struct {
      int *fd;
    } openat;
    struct {
      struct statx *statx_buff;
    } statx;
    struct {
      sds buffer;
      int client_fd;
    } send;
    struct {
      int *consumed;
    } splice;
    splice_all_context splice_all;
    struct {
      int *fd;
    } close;
    void *local;
  };
} logical_async_context;

void async_io_setup();
void async_io_shutdown();

void async_io_update_values(int server_fd, router_t *rtr);

void handle_accept_submission();
void handle_accept_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx);

void handle_recv_submission(int client_fd, recv_context *recv_ctx);
void handle_recv_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx);

void handle_openat_submission(sds path, int *fd, protothread_state *pt_state);
void handle_openat_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx);

void handle_statx_submission(int fd, struct statx *statx_buff,
                             protothread_state *pt_state);
void handle_statx_completion(struct io_uring_cqe *cqe,
                             logical_async_context *ctx);

void handle_send_submission(sds buffer, int client_fd,
                            protothread_state *pt_state);
void handle_send_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx);

void handle_splice_submission(int *consumed, int fd_in, int64_t off_in,
                              int fd_out, int64_t off_out, unsigned int nbytes,
                              unsigned int splice_flags,
                              protothread_state *pt_state);
void handle_splice_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx);

void handle_splice_all_submission(splice_all_context splice_all_ctx,
                                  protothread_state *pt_state);
void handle_splice_all_completion(struct io_uring_cqe *cqe,
                                  logical_async_context *ctx);

void handle_close_submission(int fd);
void handle_close_completion(struct io_uring_cqe *cqe,
                             logical_async_context *ctx);

void async_io_process();

typedef struct open_file_locals {
  sds path;
  file_t *file;
} open_file_locals;

void async_io_open_file(protothread_state *pt_state);

typedef struct send_buffer_locals {
  sds buffer;
  int client_fd;
} send_buffer_locals;

void async_io_send_buffer(protothread_state *pt_state);

typedef struct sendfile_locals {
  int client_fd;
  file_t *file;
  splice_all_context splice_ctx;
} sendfile_locals;

void async_io_sendfile(protothread_state *pt_state);

#define ASYNC_CALL(name, init_locals)                                          \
  do {                                                                         \
    locals->name##_state =                                                     \
        cmem_new(protothread_state, .caller = state,                           \
                 .locals = cmem_new(name##_locals, init_locals));              \
    PT_WAIT(state, async_io_##name(locals->name##_state));                     \
    cmem_free(locals->name##_state->locals);                                   \
    cmem_free(locals->name##_state);                                           \
  } while (0)