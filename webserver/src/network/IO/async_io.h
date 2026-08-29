#pragma once

#include <liburing.h>

#include "core/containers/string.h"

typedef enum async_op_type {
  async_op_type_accept,
  async_op_type_recv,
  async_op_type_openat,
  async_op_type_send,
  async_op_type_sendfile,
  async_op_type_close,
  async_op_type_statx,
} async_op_type;

// This is what you store in set_data() - contains everything needed
// to resume when the operation completes
typedef struct logical_async_context {
  async_op_type op_type;
  void *resume_point;
  void *internal;
  void *local;
} logical_async_context;

typedef struct FILE {
  int fd;
  string name;
  string path;
  struct statx statx_buff;
} FILE;

// NOTE: If not appropriately called, may cause weird crashes.
void async_io_setup();
void async_io_shutdown();

void async_io_process();

void async_io_open_file(string path, FILE *file);

#define ASYNC()

void handle_accept_submission(logical_async_context *ctx);
void handle_accept_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx);

void handle_recv_submission(logical_async_context *ctx);
void handle_recv_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx);

void handle_send_submission(logical_async_context *ctx);
void handle_send_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx);

void handle_sendfile_submission(logical_async_context *ctx);
void handle_sendfile_completion(struct io_uring_cqe *cqe,
                                logical_async_context *ctx);

void handle_close_submission(struct io_uring *ring, int fd);
void handle_close_completion(logical_async_context *ctx);
