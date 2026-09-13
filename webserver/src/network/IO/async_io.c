#include "async_io.h"

#include "core/containers/LRU_cache.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/util/profiler.h"
#include "core/util/util.h"
#include "network/http/request.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <linux/stat.h>
#include <netinet/in.h>
#include <stddef.h>

typedef struct uring_state {
  LRU_cache *file_cache;
  struct io_uring ring;
  int srv_fd;
  u64 connections;
  router *rtr;
} uring_state;

#define BUFFER_SIZE 1892

static uring_state state;

struct io_uring_sqe *io_uring_get_sqe_wrapper() {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&state.ring);
  if (sqe == nullptr) {
    LOG_ERROR("uring_get_and_check_sqe - sqe was null.");
  }
  return sqe;
}

void file_eviction_handler(void *fd) { handle_close_submission(*((int *)fd)); }

// TODO: pass uring config
void async_io_setup(int srv_fd, u64 connections, router *rtr) {
  state.srv_fd = srv_fd;
  state.connections = connections;
  state.rtr = rtr;

  // uring setup
  struct io_uring_params params;
  cmem_zmem(&params, sizeof(params));
  params.flags |= IORING_SETUP_SQPOLL | IORING_SETUP_SQ_AFF;
  params.sq_thread_cpu = 3;
  params.sq_thread_idle = 2000; // 2s timeout

  int ret = io_uring_queue_init_params(
      64, &state.ring,
      &params); // TODO: solve the magic number issue, maybe make it computed so
                // that there is a ratio between submission and completion queue
                // lengths
  if (ret < 0) {
    LOG_FATAL("async_io_setup - io_uring init failed.");
    return;
  }

  // file_cache setup
  state.file_cache = LRU_cache_create(16, sizeof(FILE), file_eviction_handler);
}

void async_io_shutdown() {
  LRU_cache_destroy(state.file_cache);
  io_uring_queue_exit(&state.ring);
}

void handle_accept_submission() {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();
  io_uring_prep_accept(sqe, state.srv_fd, nullptr, nullptr, 0);

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_accept;

  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_accept_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx) {
  if (cqe->res < 0) {
    LOG_ERROR("handle_accept_completion - accept failed: %d", cqe->res);
  } else {
    state.connections--;
    handle_recv_submission(cqe->res, nullptr);
  }
  if (state.connections > 0) {
    handle_accept_submission();
  }
}

void handle_recv_submission(int client_fd, recv_context *recv_ctx) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_recv;
  ctx->recv.client_fd = client_fd;
  if (recv_ctx) {
    cmem_mcpy(&ctx->recv, recv_ctx, sizeof(recv_context));
  } else {
    ctx->recv.buffer = cmem_alloc(BUFFER_SIZE);
    ctx->recv.offset = 0;
  }

  io_uring_prep_recv(sqe, ctx->recv.client_fd,
                     ctx->recv.buffer + ctx->recv.offset,
                     BUFFER_SIZE - ctx->recv.offset, 0);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

// TODO: Eventually should flush all requests within a read, not just the first
// one.
void handle_recv_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx) {
  int bytes_read = cqe->res;
  if (bytes_read < 0) {
    LOG_ERROR("handle_recv_completion - Standard linux error.");
    return;
  } else if (bytes_read == 0) {
    LOG_DEBUG("handle_recv_completion - Client closed connection. Feature not "
              "yet implemeented.");
    return;
  }

  int parse_result = request_parse(&ctx->recv.request, ctx->recv.buffer,
                                   ctx->recv.offset + bytes_read);

  if (parse_result < 0) {
    // TODO: send error
    LOG_ERROR("handle_recv_completion - Error parsing, need to figure out "
              "how to flush the buffer safely.");
    return;
  } else { // Not everything arrived, need more data.
    if (parse_result == 0) {
      if (ctx->recv.offset + bytes_read > BUFFER_SIZE) {
        LOG_ERROR("handle_recv_completion - Attempted to overflow buffer.");
      } else {
        ctx->recv.offset += bytes_read;
      }

      handle_recv_submission(ctx->recv.client_fd, &ctx->recv);
      cmem_free(ctx);
      return;
    } else {
      ctx->recv.offset += bytes_read;
      cmem_mcpy(ctx->recv.buffer,
                ctx->recv.buffer + ctx->recv.offset + bytes_read,
                BUFFER_SIZE - (ctx->recv.offset + bytes_read));
      ctx->recv.offset -= parse_result;
      router_handle_request(state.rtr, &ctx->recv.request, ctx->recv.client_fd);
    }
  }
}

void handle_openat_submission(string path, int *fd,
                              protothread_state *pt_state) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_openat;
  ctx->openat.fd = fd;
  ctx->pt_state = pt_state;

  io_uring_prep_openat(sqe, 0, path, 0, O_RDONLY);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_openat_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx) {
  if (cqe->res <= 0) {
    LOG_ERROR("handle_openat_completion - Failed.");
    return;
  }

  *ctx->openat.fd = cqe->res;

  ctx->pt_state->self(ctx->pt_state);
}

// NOTE: does not consume the buffer
void handle_send_submission(int client_fd, const char *buffer, size_t size) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_send;
  ctx->send.client_fd = client_fd;
  ctx->send.buffer = buffer; // NOTE: could maybe make this const?
  ctx->send.size = size;

  io_uring_prep_send(sqe, ctx->send.client_fd, ctx->send.buffer, ctx->send.size,
                     0);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_send_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx) {
  if (cqe->res < 0) {
    LOG_ERROR("error?");
    return;
  }

  ctx->send.buffer += cqe->res;
  ctx->send.size -= cqe->res;

  if (ctx->send.size > 0) {
    handle_send_submission(ctx->send.client_fd, ctx->send.buffer,
                           ctx->send.size);
  }
}

void handle_splice_submission(int file_fd, int client_fd) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_sendfile;

  io_uring_prep_splice(sqe, file_fd, 0, client_fd, 0, 0,
                       0); // NOTE: You could eventually use the out offset?
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_splice_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx) {}

void handle_close_submission(int fd) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = cmem_alloc(
      sizeof(logical_async_context)); // NOTE: not all of these need the large
                                      // ctx, should seperate into
                                      // expanded/unexpanded versions eventually
  ctx->op_type = uring_op_type_close;

  io_uring_prep_close(sqe, fd);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_close_completion(struct io_uring_cqe *cqe,
                             logical_async_context *ctx) {
  cmem_free(ctx);
}

void handle_statx_submission(int fd, struct statx *statx_buff,
                             protothread_state *pt_state) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&state.ring);

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_statx;
  ctx->statx.statx_buff = statx_buff;
  ctx->pt_state = pt_state;

  io_uring_prep_statx(sqe, AT_FDCWD, fd, 0, STATX_ALL, ctx->statx.statx_buff);

  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_statx_completion(struct io_uring_cqe *cqe,
                             logical_async_context *ctx) {
  if (cqe->res < 0) {
    LOG_ERROR("handle_statx_completion - statx failed");
    return;
  }

  ctx->pt_state->self(ctx->pt_state);
}

void async_io_process() {
  struct io_uring_cqe *cqe;

  while (io_uring_peek_cqe(&state.ring, &cqe) == 0) {
    logical_async_context *ctx = (logical_async_context *)cqe->user_data;

    switch (ctx->op_type) {
    case uring_op_type_accept:
      handle_accept_completion(cqe, ctx);
      break;
    case uring_op_type_recv:
      handle_recv_completion(cqe, ctx);
      break;
    case uring_op_type_send:
      handle_send_completion(cqe, ctx);
      break;
    case uring_op_type_openat:
      handle_openat_completion(cqe, ctx);
      break;
    case uring_op_type_close:
      handle_close_completion(cqe, ctx);
    case uring_op_type_statx:
      handle_statx_completion(cqe, ctx);
    default:
      break;
    }

    io_uring_cqe_seen(&state.ring, cqe);
  }
}

void async_io_open_file(open_file_ctx *of_ctx) {
  // Cache check to maybe skip async
  // Figure out how to not do this everytime?????
  FILE *temp_file = LRU_cache_get(state.file_cache, of_ctx->path);
  if (temp_file != nullptr) {
    cmem_mcpy(of_ctx->file, temp_file, sizeof(FILE));
    return;
  }

  PT_BEGIN(&of_ctx->state, async_io_open_file); // NOTE: Figure this out, RESUME

  // Fill out syncronous parts
  of_ctx = cmem_alloc(sizeof(FILE));
  string *path_shards = str_split_at_lit(of_ctx->path, "/");
  of_ctx->file->name =
      str_dup(path_shards[*darray_get_length(path_shards) - 1]);
  darray_destroy_string_helper(path_shards);

  PT_WAIT(&of_ctx->state, handle_openat_submission(
                              of_ctx->path, &of_ctx->file->fd, &of_ctx->state));
  PT_WAIT(&of_ctx->state,
          handle_statx_submission(of_ctx->file->fd, &of_ctx->file->statx_buff,
                                  &of_ctx->state));

  // Add to cache

  PT_END(&of_ctx->state);
}

void async_io_send_buffer(string str) {}

void async_io_sendfile(int fd) {}