#include "async_io.h"

#include "core/memory/cmem.h"
#include "core/util/protothread.h"
#include "network/IO/ring_helper.h"

// ==== LIFETIME ================
bool async_io_init();
void async_io_shutdown();
void async_io_tick();

// ==== FILE SYSTEM =============
void async_io_open_file(protothread_state *pt);
void async_io_close_file(file_t *f);

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

  pt->self(pt);
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

  // This is where I would create a connection struct, maybe allow for some custom hook for this one as well, same as the validation.

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