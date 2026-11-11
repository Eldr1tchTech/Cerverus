#include "ring_helper.h"

#include "core/util/logger.h"

#include <string.h>

// TODO: Introduce some mechanism to calculate as many of these as possible programmatically.
#define RING_SQ_ENTRIES 256
// Completions can pile up while the loop is busy (SQPOLL posts them on its
// own), and every open connection can hold an in-flight op. A full CQ means
// overflow or lost completions, i.e. hung connections. Memory is cheap.
#define RING_CQ_ENTRIES 4096
// Pin the SQPOLL kernel thread away from the main thread's core.
// Must be a valid CPU index on this machine.
#define RING_SQ_THREAD_CPU 3
#define RING_SQ_THREAD_IDLE_MS 2000

typedef struct ring_helper_g {
  struct io_uring ring;
} ring_helper_g;

static ring_helper_g state;

bool ring_helper_init(void) {
  struct io_uring_params params = {0};
  params.flags = IORING_SETUP_SQPOLL | IORING_SETUP_SQ_AFF | IORING_SETUP_CQSIZE;
  params.sq_thread_cpu = RING_SQ_THREAD_CPU;
  params.sq_thread_idle = RING_SQ_THREAD_IDLE_MS;
  params.cq_entries = RING_CQ_ENTRIES;

  int ret = io_uring_queue_init_params(RING_SQ_ENTRIES, &state.ring, &params);
  if (ret < 0) {
    LOG_FATAL("ring_helper_init - io_uring init failed: %s", strerror(-ret));
    return false;
  }

  return true;
}

void ring_helper_shutdown(void) { io_uring_queue_exit(&state.ring); }

struct io_uring_sqe *ring_helper_get_sqe(void) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&state.ring);
  if (sqe)
    return sqe;

  // SQ full. Publish the tail so the kernel can consume entries, then (under
  // SQPOLL) sleep until the poll thread frees a slot. sqring_wait is a no-op
  // without SQPOLL, where the submit alone frees the slots.
  io_uring_submit(&state.ring);
  io_uring_sqring_wait(&state.ring);

  sqe = io_uring_get_sqe(&state.ring);
  if (!sqe)
    LOG_ERROR("ring_helper_get_sqe - SQ still full after submit and wait.");
  return sqe;
}

// Only dispatches. Cleanup, including error cleanup, is the completion's job:
// only it knows what its op owns, and resuming or freeing here as well would
// double-run it.
static void dispatch(ring_helper_op_data_t *op_data, int res, unsigned flags) {
  op_data->completion_fn(op_data, res, flags);
}

static unsigned drain(void) {
  unsigned head, n = 0;
  struct io_uring_cqe *cqe;

  io_uring_for_each_cqe(&state.ring, head, cqe) {
    n++;

    // CQEs we didn't tag (user_data 0), or liburing's own internal timeout
    // marker, have no op behind them.
    if (cqe->user_data == 0 || cqe->user_data == LIBURING_UDATA_TIMEOUT)
      continue;

    // Dispatch may queue new SQEs but cannot invalidate this cqe: the head
    // is only advanced below.
    dispatch((ring_helper_op_data_t *)cqe->user_data, cqe->res, cqe->flags);
  }

  io_uring_cq_advance(&state.ring, n);
  return n;
}

void ring_helper_tick(bool block) {
  unsigned n = drain();

  // Submit AFTER draining so ops queued by completions go out now, not on
  // the next tick. Only sleep if there was nothing to do; when we handled
  // completions, more work was probably queued, so just submit and loop.
  int ret = io_uring_submit_and_wait(&state.ring, (block && n == 0) ? 1 : 0);
  if (ret < 0 && ret != -EINTR && ret != -EAGAIN && ret != -EBUSY)
    LOG_ERROR("ring_helper_tick - submit_and_wait: %s", strerror(-ret));
}

void ring_helper_resume_pt(ring_helper_op_data_t *op_data, int res,
                           unsigned flags) {
  (void)flags;
  protothread_state *pt = op_data->pt;
  pt->error = res < 0 ? -res : 0;
  pt->self(pt);
}