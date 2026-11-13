#include "async_io.h"

#include "core/containers/LRU_cache.h"
#include "core/memory/cmem.h"
#include "core/util/protothread.h"
#include "defines.h"
#include "network/IO/ring_helper.h"
#include <fcntl.h>
#include <liburing/io_uring.h>
#include <linux/stat.h>

typedef struct async_io_g {
  LRU_cache_t *cache;
  // TODO: add a pool for file_t objects
} async_io_g;

static async_io_g state;

// ==== LIFETIME ================

// TODO: Change the LRU_cache so that the memory of an freed slot somehow stays
// active??? it is freed early existing references to said file will point to
// random places in the cache...
void file_cache_eviction_handler_fn(void *item) { async_io_close_file(item); }

bool async_io_init() {
  if (!ring_helper_init(true))
    return false;

  state.cache =
      LRU_cache_create(64, sizeof(file_t *), file_cache_eviction_handler_fn);

  return true;
}

void async_io_shutdown() {
  LRU_cache_destroy(state.cache);
  ring_helper_shutdown();
}

void async_io_tick() { ring_helper_tick(true); }

// ==== FILE SYSTEM =============
void open_file_completion_fn(ring_helper_op_data_t *op_data, int res,
                             unsigned flags) {
  protothread_state *pt = op_data->pt;

  
}

void async_io_open_file(protothread_state *pt) {
  async_open_file_args_t *locals = pt->locals;

  PT_BEGIN(pt, async_io_open_file);

  if ((locals->f = LRU_cache_get(state.cache, sdstosv(locals->path)))) {
    PT_END(pt);
  }

  ring_helper_op_data_t *op_data =
      cmem_new(ring_helper_op_data_t, .completion_fn = open_file_completion_fn,
               .pt = pt);

  PT_WAIT(pt, ring_helper_queue(openat_direct, op_data, AT_FDCWD, locals->path,
                                O_RDONLY, 0, IORING_FILE_INDEX_ALLOC));

  PT_WAIT(pt, ring_helper_queue(statx, op_data, -1, "", AT_EMPTY_PATH, STATX_SIZE, &locals->f->statx_buff));

  PT_END(pt);
}

// true if closed, otherwise false
bool async_io_close_file(file_t *f) {}

// ==== NETWORK =================
void socket_completion_fn(ring_helper_op_data_t *op_data, int res,
                          unsigned flags) {
  protothread_state *pt = op_data->pt;

  if (res < 0) {
    pt->error = -res;
    goto exit;
  }
  pt->error = 0;

  ((async_socket_args_t *)pt->locals)->sock = res;

exit:
  pt->self(pt);
  return;
}

void async_io_socket(protothread_state *pt) {
  async_socket_args_t *locals = pt->locals;

  PT_BEGIN(pt, async_io_socket);

  ring_helper_op_data_t *op_data = cmem_new(
      ring_helper_op_data_t, .completion_fn = socket_completion_fn, .pt = pt);

  PT_WAIT(pt, ring_helper_queue(socket_direct_alloc, op_data, locals->domain,
                                locals->type, locals->protocol, locals->flags));
  if (pt->error) {
    goto exit;
  }

exit:
  PT_END(pt);
  return;
}

void accept_completion_fn(ring_helper_op_data_t *op_data, int res,
                          unsigned flags) {
  protothread_state *pt = op_data->pt;

  if (res < 0) {
    pt->error = -res;
    goto exit;
  }
  pt->error = 0;

  // This is where I would create a connection struct, maybe allow for some
  // custom hook for this one as well, same as the validation.

exit:
  pt->self(pt);
  return;
}

// where do I really store the
void async_io_multishot_accept(protothread_state *pt) {
  async_multishot_accept_args_t *locals = pt->locals;

  PT_BEGIN(pt, async_io_multishot_accept);

  ring_helper_op_data_t *op_data = cmem_new(
      ring_helper_op_data_t, .completion_fn = accept_completion_fn, .pt = pt);

  PT_WAIT(pt, ring_helper_queue(multishot_accept_direct, op_data, locals->fd,
                                locals->addr, locals->addrlen, locals->flags));
  if (pt->error) {
    goto exit;
  }

exit:
  PT_END(pt);

  pt->self(pt);
  return;
}

void async_io_recv(protothread_state *pt);
void async_io_send_buffer(protothread_state *pt);
void async_io_sendfile(protothread_state *pt);