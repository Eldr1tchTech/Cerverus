#pragma once

#include <liburing.h>
#include <stddef.h>

#include "core/containers/string.h"
#include "core/util/protothread.h"

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
typedef void (*async_resume_fn)(struct logical_async_context *ctx);

typedef struct logical_async_context {
  protothread_state pt_state;
  async_op_type op_type;
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

void async_io_process();

typedef struct open_file_ctx {
  protothread_state state;

  string path;
  FILE *file;

  protothread_state caller_ctx;
} open_file_ctx;

void async_io_open_file(open_file_ctx *of_ctx);

void async_io_send_buffer(string str);

void async_io_sendfile(int fd);