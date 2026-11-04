#include "async_io.h"

#include "core/containers/LRU_cache.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/util/profiler.h"
#include "core/util/protothread.h"
#include "core/vendor/sds.h"
#include "defines.h"
#include "network/http/request.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <linux/stat.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#define TEMP_CONNECTIONS 16
#define SPLICE_MAX_SIZE (64 * 1024)

typedef struct uring_state {
  LRU_cache *file_cache;
  struct io_uring ring;
  int srv_fd;
  u64 connections;
  router_t *rtr;
} uring_state;

#define RECV_CHUNK 4096
#define MAX_REQUEST (16 * 1024)

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
void async_io_setup() {
  state.connections = TEMP_CONNECTIONS;

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
  state.file_cache =
      LRU_cache_create(16, sizeof(file_t), file_eviction_handler);
}

void async_io_shutdown() {
  LRU_cache_destroy(state.file_cache);
  io_uring_queue_exit(&state.ring);
}

void async_io_update_values(int server_fd, router_t *rtr) {
  state.srv_fd = server_fd;
  state.rtr = rtr;
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
  LOG_INFO("received an accept request.");
  if (cqe->res < 0) {
    LOG_ERROR("handle_accept_completion - accept failed: %d", cqe->res);
  } else {
    // TODO: Remember to somehow reincrement this at the correct point later in
    // the code.
    state.connections--;
    handle_recv_submission(cqe->res, nullptr);
  }
  if (state.connections > 0) {
    handle_accept_submission();
  }
}

void handle_recv_submission(int client_fd, recv_context *recv_ctx) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  // TODO: maybe zero initialize
  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_recv;

  if (recv_ctx) {
    ctx->recv = *recv_ctx;
  } else {
    ctx->recv.client_fd = client_fd;
    ctx->recv.buffer = sdsempty();
  }

  ctx->recv.buffer = sdsMakeRoomFor(ctx->recv.buffer, RECV_CHUNK);

  size_t len = sdslen(ctx->recv.buffer);
  io_uring_prep_recv(sqe, ctx->recv.client_fd, ctx->recv.buffer + len,
                     sdsavail(ctx->recv.buffer), 0);
  io_uring_sqe_set_data(sqe, ctx);
  io_uring_submit(&state.ring);
}

void handle_recv_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx) {
  int bytes_read = cqe->res;
  if (bytes_read <= 0) {
    sdsfree(ctx->recv.buffer);
    cmem_free(ctx);
    return;
  }

  sdsIncrLen(ctx->recv.buffer, bytes_read);
  ctx->recv.request._raw = ctx->recv.buffer;

  int parse_result = request_parse(&ctx->recv.request);

  if (parse_result < 0 || sdslen(ctx->recv.buffer) > MAX_REQUEST) {
    // send 400/431 and close
    sdsfree(ctx->recv.buffer);
    cmem_free(ctx);
    LOG_INFO("recv completion finished.");
    return;
  }

  if (parse_result == 0) {
    handle_recv_submission(ctx->recv.client_fd, &ctx->recv);
    cmem_free(ctx);
    return;
  }

  LOG_INFO("recv completion finished, successfully.\n"
           "%.*s",
           sdslen(ctx->recv.request._raw), ctx->recv.request._raw);
  router_handle_request(state.rtr, &ctx->recv.request, ctx->recv.client_fd);
}

void handle_openat_submission(sds path, int *fd, protothread_state *pt_state) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_openat;
  ctx->openat.fd = fd;
  ctx->pt_state = pt_state;

  LOG_INFO("openat path: %.*s", sdslen(path), path);

  io_uring_prep_openat(sqe, AT_FDCWD, path, 0, O_RDONLY);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_openat_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx) {
  if (cqe->res <= 0) {
    LOG_ERROR("handle_openat_completion - Failed. %d", cqe->res);
    return;
  }

  *ctx->openat.fd = cqe->res;

  ctx->pt_state->self(ctx->pt_state);
}

void handle_send_submission(sds buffer, int client_fd,
                            protothread_state *pt_state) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_send;
  ctx->send.client_fd = client_fd;
  ctx->send.buffer = buffer;
  ctx->pt_state = pt_state;

  io_uring_prep_send(sqe, ctx->send.client_fd, ctx->send.buffer,
                     sdslen(ctx->send.buffer), 0);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_send_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx) {
  if (cqe->res < 0) {
    LOG_ERROR("error?");
    return;
  }

  sdsrange(ctx->send.buffer, cqe->res, -1);

  if (sdslen(ctx->send.buffer) > 0) {
    handle_send_submission(ctx->send.buffer, ctx->send.client_fd,
                           ctx->pt_state);
    return;
  }

  sdsfree(ctx->send.buffer);
  ctx->pt_state->self(ctx->pt_state);
}

void handle_splice_submission(int *consumed, int fd_in, int64_t off_in,
                              int fd_out, int64_t off_out, unsigned int nbytes,
                              unsigned int splice_flags,
                              protothread_state *pt_state) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_splice;
  ctx->splice.consumed = consumed;
  ctx->pt_state = pt_state;

  io_uring_prep_splice(sqe, fd_in, off_in, fd_out, off_out, nbytes,
                       splice_flags);

  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_splice_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx) {
  if (cqe->res < 0) {
    ctx->splice.consumed = nullptr;
    LOG_ERROR("handle_splice_completion - %s", strerror(-cqe->res));
    ctx->pt_state->self(ctx->pt_state);
  }

  *ctx->splice.consumed = cqe->res;
  ctx->pt_state->self(ctx->pt_state);
}

void handle_splice_all_submission(splice_all_context splice_all_ctx,
                                  protothread_state *pt_state) {
  // Setup ctx
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();
  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_splice_all;
  ctx->splice_all = splice_all_ctx;
  ctx->pt_state = pt_state;

  if (ctx->splice_all.is_draining) {
    // Drain pipe
    io_uring_prep_splice(sqe, ctx->splice_all.pipe[0], -1,
                         ctx->splice_all.fd_out, -1, ctx->splice_all.in_pipe,
                         0);
  } else {
    // Fill pipe
    io_uring_prep_splice(sqe, ctx->splice_all.fd_in, ctx->splice_all.off_in,
                         ctx->splice_all.pipe[1], -1,
                         MIN(ctx->splice_all.nbytes, SPLICE_MAX_SIZE) -
                             ctx->splice_all.in_pipe,
                         0);
  }

  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_splice_all_completion(struct io_uring_cqe *cqe,
                                  logical_async_context *ctx) {
  if (cqe->res < 0) {
    LOG_ERROR("handle_splice_all_completion - %s", strerror(-cqe->res));
    ctx->pt_state->self(ctx->pt_state);
  }

  // Update ctx
  if (ctx->splice_all.is_draining) {
    ctx->splice_all.in_pipe -= cqe->res;

    if (ctx->splice_all.in_pipe == ctx->splice_all.nbytes == 0) {
      ctx->pt_state->self(ctx->pt_state);
    } else {
      ctx->splice_all.is_draining = false;
      handle_splice_all_submission(ctx->splice_all, ctx->pt_state);
    }
  } else {
    ctx->splice_all.nbytes -= cqe->res;
    ctx->splice_all.in_pipe += cqe->res;
    ctx->splice_all.is_draining = true;
    handle_splice_all_submission(ctx->splice_all, ctx->pt_state);
  }
}

void handle_close_submission(int fd) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
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
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = uring_op_type_statx;
  ctx->statx.statx_buff = statx_buff;
  ctx->pt_state = pt_state;

  io_uring_prep_statx(sqe, fd, "", AT_EMPTY_PATH, STATX_ALL,
                      ctx->statx.statx_buff);

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

  int res = io_uring_peek_cqe(&state.ring, &cqe);

  while (res == 0) {
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
      break;
    case uring_op_type_statx:
      handle_statx_completion(cqe, ctx);
      break;
    case uring_op_type_splice:
      handle_splice_completion(cqe, ctx);
      break;
    case uring_op_type_splice_all:
      handle_splice_all_completion(cqe, ctx);
      break;
    default:
      LOG_ERROR("async_io_process - Unknown uring_op_type.");
      break;
    }

    io_uring_cqe_seen(&state.ring, cqe);

    res = io_uring_peek_cqe(&state.ring, &cqe);
  }

  if (res < 0 && res != -EAGAIN) {
    LOG_ERROR("async_io_process - Error emitted: %i", res);
  }
}

void async_io_open_file(protothread_state *pt_state) {
  open_file_locals *locals = (open_file_locals *)pt_state->locals;

  // Cache check to maybe skip async
  // Figure out how to not do this everytime?????
  file_t *temp_file = LRU_cache_get(state.file_cache, sdstosv(locals->path));
  if (temp_file != nullptr) {
    cmem_mcpy(locals->file, temp_file, sizeof(file_t));
    return;
  }

  PT_BEGIN(pt_state, async_io_open_file);

  // Fill out synchronous parts
  int count;
  sds *path_shards =
      sdssplitlen(locals->path, sdslen(locals->path), "/", 1, &count);
  locals->file->name = sdsdup(path_shards[count - 1]);
  sdsfreesplitres(path_shards, count);

  PT_WAIT(pt_state,
          handle_openat_submission(locals->path, &locals->file->fd, pt_state));
  PT_WAIT(pt_state, handle_statx_submission(
                        locals->file->fd, &locals->file->statx_buff, pt_state));

  // Add to cache
  LRU_cache_add(
      state.file_cache, sdstosv(locals->path),
      locals->file); // NOTE: may be necessary to adopt a facotry-like approach
                     // for duplicating complex structs, especially if they
                     // include strings and the like

  PT_END(pt_state);
}

void async_io_send_buffer(protothread_state *pt_state) {
  send_buffer_locals *locals = pt_state->locals;

  PT_BEGIN(pt_state, async_io_send_buffer);

  PT_WAIT(pt_state, handle_send_submission(sdsdup(locals->buffer),
                                           locals->client_fd, pt_state));

  PT_END(pt_state);
}

void async_io_sendfile(protothread_state *pt_state) {
  sendfile_locals *locals = pt_state->locals;

  PT_BEGIN(pt_state, async_io_sendfile);

  // TODO: Make this a uring syscall.
  locals->splice_ctx =
      (splice_all_context){.fd_in = locals->file->fd,
                           .fd_out = locals->client_fd,
                           .off_in = 0,
                           .nbytes = locals->file->statx_buff.stx_size,
                           .is_draining = false};

  if (pipe2(locals->splice_ctx.pipe, O_CLOEXEC))
    LOG_ERROR("async_io_sendfile - pipe2 failed.");

  PT_WAIT(pt_state, handle_splice_all_submission(locals->splice_ctx, pt_state));

  PT_END(pt_state);
}