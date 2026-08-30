#include "async_io.h"

#include "core/containers/LRU_cache.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/util/profiler.h"
#include "network/http/request.h"
#include "network/http/response.h"

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
} uring_state;

static uring_state state;

void file_eviction_handler(void *fd) {
  handle_close_submission(&state.ring, *((int *)fd));
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
  state.file_cache = LRU_cache_create(16, sizeof(FILE), file_eviction_handler);
}

void async_io_shutdown() {
  LRU_cache_destroy(state.file_cache);
  io_uring_queue_exit(&state.ring);
}

// TODO: Make this just use the normal allocator, don't want weird bugs yet.
void handle_accept_submission() {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&state.ring);

  // TODO: Finish figuring this out...

  io_uring_prep_multishot_accept(sqe, , nullptr, nullptr, 0);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_accept_completion(struct io_uring_cqe *cqe,
                              logical_async_context *ctx) {
  if (cqe->res < 0) {
    LOG_ERROR("handle_accept_completion - accept failed: %d", cqe->res);
  } else {
    uring_context *conn_ctx = cmem_alloc(sizeof(uring_context));
    conn_ctx->srv = ctx->srv;
    conn_ctx->op_type = uring_op_type_recv;
    conn_ctx->client.fd = cqe->res;
    conn_ctx->request.offset = 0;
    handle_recv_submission(conn_ctx);
  }

  // The multishot op terminated — it must be re-armed.
  if (!(cqe->flags & IORING_CQE_F_MORE)) {
    pool_allocator_free(ctx->srv->uring.pool_alloc_ctx, ctx);
    handle_accept_submission(ctx->srv);
  }
}

void handle_recv_submission(uring_context *ctx) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->uring.ring);
  ctx->op_type = uring_op_type_recv;

  io_uring_prep_recv(sqe, ctx->client.fd,
                     ctx->request.buffer + ctx->request.offset,
                     BUFFER_SIZE - ctx->request.offset, 0);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&ctx->srv->uring.ring);
}

// TODO: Eventually should flush all requests within a read, not just the first
// one.
void handle_recv_completion(struct io_uring_cqe *cqe, uring_context *ctx) {
  int bytes_read = cqe->res;
  if (bytes_read < 0) {
    LOG_ERROR("handle_recv_completion - Standard linux error.");
    return;
  } else if (bytes_read == 0) {
    LOG_DEBUG("handle_recv_completion - Client closed connection. Feature not "
              "yet implemeented.");
    return;
  }

  int parse_result = request_parse(&ctx->request.request, ctx->request.buffer,
                                   ctx->request.offset + bytes_read);

  if (parse_result < 0) {
    // TODO: send error
    LOG_ERROR("handle_recv_completion - Error parsing, need to figure out "
              "how to flush the buffer safely.");
    return;
  } else { // Not everything arrived, need more data.
    if (parse_result == 0) {
      if (ctx->request.offset + bytes_read > BUFFER_SIZE) {
        LOG_ERROR("handle_recv_completion - Attempted to overflow buffer.");
      } else {
        ctx->request.offset += bytes_read;
      }

      handle_recv_submission(ctx);
      return;
    } else {
      ctx->request.offset += bytes_read;
      cmem_mcpy(ctx->request.buffer,
                ctx->request.buffer + ctx->request.offset + bytes_read,
                BUFFER_SIZE - (ctx->request.offset + bytes_read));
      ctx->request.offset -= parse_result;
      router_handle_request(ctx->srv->rtr, &ctx->request.request,
                            ctx->client.fd);
    }
  }
}

void handle_openat_submission(logical_async_context *ctx, string *path) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->uring.ring);
  ctx->op_type = uring_op_type_openat;

  io_uring_prep_openat(sqe, 0, path, 0, O_RDONLY);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&ctx->srv->uring.ring);
}

void handle_openat_completion(struct io_uring_cqe *cqe, uring_context *ctx) {
  if (cqe->res <= 0) {
    LOG_ERROR("handle_openat_completion - Failed.");
    return;
  }

  ctx->file_fd = cqe->res;
}

void handle_send_submission(uring_context *ctx) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->uring.ring);
  ctx->op_type = uring_op_type_send;

  if (!ctx->response.buffer) {
    ctx->response.buffer = response_serialize(
        ctx->response.response); // TODO: Eventually have this fill the length.
    ctx->response.length = strlen(ctx->response.buffer);
    ctx->response.offset = 0;
  }

  io_uring_prep_send(sqe, ctx->client.fd,
                     ctx->response.buffer +
                         (sizeof(char) * ctx->response.offset),
                     ctx->response.length, 0);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&ctx->srv->uring.ring);
}

void handle_send_completion(struct io_uring_cqe *cqe, uring_context *ctx) {
  if (cqe->res < 0) {
    LOG_ERROR("error?");
    return;
  }

  ctx->response.offset += cqe->res;

  if (ctx->response.length > ctx->response.offset) {
    handle_send_submission(ctx);
    return;
  }

  handle_close_submission(&ctx->srv->uring.ring, ctx->client.fd);
}

void handle_sendfile_submission(uring_context *ctx) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->uring.ring);
  ctx->op_type = uring_op_type_sendfile;

  io_uring_prep_splice(sqe, ctx->file_fd, 0, ctx->client.fd, 0, 0, 0);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&ctx->srv->uring.ring);
}

void handle_sendfile_completion(struct io_uring_cqe *cqe,
                                logical_async_context *ctx) {}

void handle_close_submission(int fd) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(state.ring);
  logical_async_context *ctx = cmem_alloc(sizeof(logical_async_context));
  ctx->op_type = async_op_type_close;

  io_uring_prep_close(sqe, fd);
  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(ring);
}

void handle_close_completion(uring_context *ctx) { cmem_free(ctx); }

void handle_statx_submission(logical_async_context *ctx, string path) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(state.ring);

  io_uring_prep_statx(sqe, AT_FDCWD, path, 0, STATX_ALL,
                      &((FILE *)ctx->internal)->statx_buff);

  io_uring_sqe_set_data(sqe, ctx);

  io_uring_submit(&state.ring);
}

void handle_statx_completion(struct io_uring_cqe *cqe,
                             logical_async_context *ctx) {
  if (cqe->res < 0) {
    LOG_ERROR("handle_statx_completion - statx failed on %s: %d",
              ctx->statx.path, cqe->res);
    return;
  }

  goto ctx->resume_point; // If I end with a goto, do I need to consume the sqe
                          // here?
}

void async_io_process() {
  struct io_uring_cqe *cqe;

  while (io_uring_peek_cqe(&state.ring, &cqe) == 0) {
    logical_async_context *ctx = (logical_async_context *)cqe->user_data;

    switch (ctx->op_type) {
    case async_op_type_accept:
      handle_accept_completion(cqe, ctx);
      break;
    case async_op_type_recv:
      handle_recv_completion(cqe, ctx);
      break;
    case async_op_type_send:
      handle_send_completion(cqe, ctx);
      break;
    case async_op_type_openat:
      handle_openat_completion(cqe, ctx);
      break;
    case async_op_type_close:
      handle_close_completion(ctx);
    case async_op_type_statx:
      handle_statx_completion(cqe, ctx);
    default:
      break;
    }

    io_uring_cqe_seen(&state.ring, cqe);
  }
}

// TODO: Create a macro that adds a pt_state on the front of a struct.
void async_io_open_file(open_file_ctx *of_ctx) {
  // Cache check to maybe skip async
  // Figure out how to not do this everytime?????
  FILE *temp_file = LRU_cache_get(state.file_cache, of_ctx->path);
  if (temp_file != nullptr) {
    cmem_mcpy(of_ctx->file, temp_file, sizeof(FILE));
    return;
  }

  PT_BEGIN(of_ctx->state, async_io_open_file);

  // Fill out syncronous parts
  file = cmem_alloc(sizeof(FILE));
  file->path = path;
  string *path_shards = str_split_at_lit(path, "/");
  file->name = str_dup(path_shards[darray_get_length(path_shards) - 1]);
  darray_destroy_string_helper(path_shards);

  PT_WAIT(of_ctx->state,
          handle_openat_submission(
              ctx)); // Why even have this be async? just pass as parameter?
                     // Also since it returns early can't the state go out of
                     // context before it is written back into?
  PT_WAIT(of_ctx->state,
          handle_statx_submission(
              ctx)); // Why even have this be async? just pass as parameter?

  PT_END(of_ctx->state);
}

void async_io_send_buffer(string str) {}

void async_io_sendfile(int fd) {}