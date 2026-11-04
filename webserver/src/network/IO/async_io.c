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

#define SPLICE_MAX_SIZE (64 * 1024)
#define RECV_CHUNK (4 * 1024)
#define MAX_REQUEST (16 * 1024)

typedef void (*completion_fn)(struct io_uring_cqe *, logical_async_context *);

static const completion_fn completion_table[uring_op_type_count] = {
#define X(n) [uring_op_type_##n] = handle_##n##_completion,
    URING_OPS(X)
#undef X
};

typedef struct uring_state {
  LRU_cache *file_cache;
  struct io_uring ring;
  int srv_fd;
  router_t *rtr;
} uring_state;

static uring_state state;

struct io_uring_sqe *io_uring_get_sqe_wrapper() {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&state.ring);
  if (sqe == nullptr) {
    LOG_ERROR("uring_get_and_check_sqe - sqe was null.");
  }
  return sqe;
}

static logical_async_context *new_ctx(uring_op_type op_type,
                                      protothread_state *pt) {
  return cmem_new(logical_async_context, .op_type = op_type, .pt = pt);
}

static void op_submit(struct io_uring_sqe *sqe, logical_async_context *ctx) {
  io_uring_sqe_set_data(sqe, ctx);
  io_uring_submit(&state.ring);
}

static void op_exit(logical_async_context *ctx) {
  protothread_state *pt = ctx->pt;
  cmem_free(ctx);
  if (pt)
    pt->self(pt);
}

#define OP_CHECK(cqe, ctx, what)                                               \
  if ((cqe)->res < 0) {                                                        \
    LOG_ERROR(what " - %s", strerror(-(cqe)->res));                            \
    op_exit((ctx));                                                            \
    return;                                                                    \
  }

void file_eviction_handler(void *fd) {
  handle_close_submission(*((int *)fd), nullptr);
}

// TODO: pass uring config
void async_io_setup() {
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
  logical_async_context *ctx = new_ctx(uring_op_type_accept, nullptr);

  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();
  io_uring_prep_accept(sqe, state.srv_fd, nullptr, nullptr, 0);

  op_submit(sqe, ctx);
}

void handle_accept_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx) {
  OP_CHECK(cqe, ctx, "handle_accept_completion");

  handle_recv_submission(cqe->res, nullptr);
  handle_accept_submission();
}

void handle_recv_submission(int client_fd, recv_context *recv_ctx) {
  logical_async_context *ctx = new_ctx(uring_op_type_recv, nullptr);

  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

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
  op_submit(sqe, ctx);
}

void handle_recv_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx) {
  // TODO: Revisit (not following schema)
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

void handle_openat_submission(sds path, int *fd, protothread_state *pt) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = new_ctx(uring_op_type_openat, pt);
  ctx->openat.fd = fd;

  LOG_INFO("openat path: %.*s", sdslen(path), path);

  io_uring_prep_openat(sqe, AT_FDCWD, path, O_RDONLY, 0);
  op_submit(sqe, ctx);
}

void handle_openat_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx) {
  OP_CHECK(cqe, ctx, "handle_openat_completion");

  *ctx->openat.fd = cqe->res;

  ctx->pt->self(ctx->pt);
}

void handle_send_submission(sds buffer, int client_fd, protothread_state *pt) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = new_ctx(uring_op_type_send, pt);
  ctx->send.client_fd = client_fd;
  ctx->send.buffer = buffer;

  io_uring_prep_send(sqe, ctx->send.client_fd, ctx->send.buffer,
                     sdslen(ctx->send.buffer), 0);
  op_submit(sqe, ctx);
}

void handle_send_completion(struct io_uring_cqe *cqe,
                            logical_async_context *ctx) {
  OP_CHECK(cqe, ctx, "handle_send_completion");

  sdsrange(ctx->send.buffer, cqe->res, -1);

  if (sdslen(ctx->send.buffer) > 0) {
    handle_send_submission(ctx->send.buffer, ctx->send.client_fd, ctx->pt);
    return;
  }

  protothread_state *pt = ctx->pt;
  sdsfree(ctx->send.buffer);
  cmem_free(ctx);
  if (pt)
    pt->self(pt);
}

void handle_splice_submission(int *consumed, int fd_in, int64_t off_in,
                              int fd_out, int64_t off_out, unsigned int nbytes,
                              unsigned int splice_flags,
                              protothread_state *pt) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = new_ctx(uring_op_type_splice, pt);
  ctx->splice.consumed = consumed;

  io_uring_prep_splice(sqe, fd_in, off_in, fd_out, off_out, nbytes,
                       splice_flags);

  op_submit(sqe, ctx);
}

void handle_splice_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx) {
  OP_CHECK(cqe, ctx, "handle_splice_completion");

  *ctx->splice.consumed = cqe->res;
  ctx->pt->self(ctx->pt);
}

void handle_splice_all_submission(splice_all_context splice_all_ctx,
                                  protothread_state *pt) {
  // Setup ctx
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();
  logical_async_context *ctx = new_ctx(uring_op_type_splice_all, pt);
  ctx->splice_all = splice_all_ctx;

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

  op_submit(sqe, ctx);
}

void handle_splice_all_completion(struct io_uring_cqe *cqe,
                                  logical_async_context *ctx) {
  OP_CHECK(cqe, ctx, "handle_splice_all_completion");

  // Update ctx
  if (ctx->splice_all.is_draining) {
    ctx->splice_all.in_pipe -= cqe->res;

    if (ctx->splice_all.in_pipe == 0 && ctx->splice_all.nbytes == 0) {
      ctx->pt->self(ctx->pt);
    } else {
      ctx->splice_all.is_draining = false;
      handle_splice_all_submission(ctx->splice_all, ctx->pt);
      return;
    }
  } else {
    ctx->splice_all.nbytes -= cqe->res;
    ctx->splice_all.in_pipe += cqe->res;
    ctx->splice_all.is_draining = true;
    handle_splice_all_submission(ctx->splice_all, ctx->pt);
    return;
  }
}

void handle_close_submission(int fd, protothread_state *pt) {
  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();

  logical_async_context *ctx = new_ctx(uring_op_type_close, pt);

  io_uring_prep_close(sqe, fd);
  op_submit(sqe, ctx);
}

void handle_close_completion(struct io_uring_cqe *cqe,
                             logical_async_context *ctx) {
  // NOTE: I don't think close can actually fail...
  OP_CHECK(cqe, ctx, "handle_close_completion");
  op_exit(ctx);
}

void handle_statx_submission(int fd, struct statx *statx_buff,
                             protothread_state *pt) {
  logical_async_context *ctx = new_ctx(uring_op_type_statx, pt);
  ctx->statx.statx_buff = statx_buff;

  struct io_uring_sqe *sqe = io_uring_get_sqe_wrapper();
  io_uring_prep_statx(sqe, fd, "", AT_EMPTY_PATH, STATX_ALL,
                      ctx->statx.statx_buff);

  op_submit(sqe, ctx);
}

void handle_statx_completion(struct io_uring_cqe *cqe,
                             logical_async_context *ctx) {
  OP_CHECK(cqe, ctx, "handle_statx_completion");

  protothread_state *pt = ctx->pt;
  cmem_free(ctx);
  pt->self(pt);
}

void async_io_process() {
  struct io_uring_cqe *cqe;

  int res = io_uring_peek_cqe(&state.ring, &cqe);

  while (res == 0) {
    logical_async_context *ctx = (logical_async_context *)cqe->user_data;

    if (ctx->op_type >= uring_op_type_count)
      LOG_ERROR("Unknown uring_op_type.");
    else
      completion_table[ctx->op_type](cqe, ctx);

    io_uring_cqe_seen(&state.ring, cqe);

    res = io_uring_peek_cqe(&state.ring, &cqe);
  }

  if (res < 0 && res != -EAGAIN) {
    LOG_ERROR("async_io_process - Error emitted: %i", res);
  }
}

void async_io_open_file(protothread_state *pt) {
  open_file_locals *locals = (open_file_locals *)pt->locals;

  // Cache check to maybe skip async
  // Figure out how to not do this everytime?????
  file_t *temp_file = LRU_cache_get(state.file_cache, sdstosv(locals->path));
  if (temp_file != nullptr) {
    cmem_mcpy(locals->file, temp_file, sizeof(file_t));
    pt->caller->self(pt->caller);
    return;
  }

  PT_BEGIN(pt, async_io_open_file);

  // Fill out synchronous parts
  int count;
  sds *path_shards =
      sdssplitlen(locals->path, sdslen(locals->path), "/", 1, &count);
  locals->file->name = sdsdup(path_shards[count - 1]);
  sdsfreesplitres(path_shards, count);

  PT_WAIT(pt, handle_openat_submission(locals->path, &locals->file->fd, pt));
  PT_WAIT(pt, handle_statx_submission(locals->file->fd,
                                      &locals->file->statx_buff, pt));

  // Add to cache
  LRU_cache_add(
      state.file_cache, sdstosv(locals->path),
      locals->file); // NOTE: may be necessary to adopt a facotry-like approach
                     // for duplicating complex structs, especially if they
                     // include strings and the like

  PT_END(pt);
}

void async_io_send_buffer(protothread_state *pt) {
  send_buffer_locals *locals = pt->locals;

  PT_BEGIN(pt, async_io_send_buffer);

  PT_WAIT(pt, handle_send_submission(sdsdup(locals->buffer), locals->client_fd,
                                     pt));

  PT_END(pt);
}

void async_io_sendfile(protothread_state *pt) {
  sendfile_locals *locals = pt->locals;

  PT_BEGIN(pt, async_io_sendfile);

  // TODO: Make this a uring syscall.
  locals->splice_ctx =
      (splice_all_context){.fd_in = locals->file->fd,
                           .fd_out = locals->client_fd,
                           .off_in = 0,
                           .nbytes = locals->file->statx_buff.stx_size,
                           .is_draining = false};

  if (pipe2(locals->splice_ctx.pipe, O_CLOEXEC))
    LOG_ERROR("async_io_sendfile - pipe2 failed.");

  PT_WAIT(pt, handle_splice_all_submission(locals->splice_ctx, pt));

  PT_END(pt);
}