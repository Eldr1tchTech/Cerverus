#pragma once

#include <liburing.h>
#include <stddef.h>

#include "core/containers/string.h"
#include "core/util/protothread.h"
#include "network/network_types.inl"
#include "network/routing/router.h"

typedef enum uring_op_type {
  uring_op_type_accept,
  uring_op_type_recv,
  uring_op_type_openat,
  uring_op_type_send,
  uring_op_type_sendfile,
  uring_op_type_close,
  uring_op_type_statx,
} uring_op_type;

typedef struct recv_context {
  int client_fd;
  char *buffer;
  size_t offset;
  request request;
} recv_context;

typedef struct send_context {

} send_context;

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
      int client_fd;
      char *buffer;
      size_t size;
    } send;
    struct {
      int file_fd;
      int client_fd;
    } splice;
    struct {
      int *fd;
    } close;
    void *local;
  };
} logical_async_context;

typedef struct FILE {
  int fd;
  string name;
  string path;
  struct statx statx_buff;
} FILE;

// NOTE: If not appropriately called, may cause weird crashes.
void async_io_setup(int srv_fd, u64 max_connections, router *rtr);
void async_io_shutdown();

void handle_accept_submission();
void handle_accept_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx);

void handle_recv_submission(int client_fd, recv_context *recv_ctx);
void handle_recv_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx);

void handle_openat_submission(string path, int *fd,
                              protothread_state *pt_state);
void handle_openat_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx);

void handle_statx_submission(int fd, struct statx *statx_buff,
                             protothread_state *pt_state);
void handle_statx_completion(struct io_uring_cqe *cqe,
                             logical_async_context *ctx);

void handle_send_submission(int client_fd, const char *buffer, size_t size);
void handle_send_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx);

void handle_splice_submission(int file_fd, int client_fd);
void handle_splice_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx);

void handle_close_submission(int fd);
void handle_close_completion(struct io_uring_cqe *cqe,
                             logical_async_context *ctx);

void async_io_process();

typedef struct open_file_ctx {
  protothread_state state;

  string path;
  FILE *file;

  protothread_state *caller_ctx;
} open_file_ctx;

void async_io_open_file(open_file_ctx *of_ctx);

void async_io_send_buffer(string str);

void async_io_sendfile(int fd);