#pragma once

// The io_uring interface layer. Knows how to: own the ring, hand out SQEs,
// and drain CQEs by calling each op's completion function.
// Knows NOTHING about accept/send/files/HTTP. Per-op state and completion
// functions live in async_io.c.
//
// Only async_io.c should include this header (the queue macro expands there
// and needs liburing's prep functions).

#include "core/util/protothread.h"

#include <liburing.h>

typedef struct ring_helper_op_data_t ring_helper_op_data_t;

// Called from ring_helper_tick for every CQE tagged with this op.
//   res   : cqe->res (negative = -errno)
//   flags : cqe->flags (check IORING_CQE_F_MORE for multishot ops: if set,
//           the op is still armed and must NOT be freed)
// The completion function is responsible for ALL cleanup, errors included.
typedef void (*ring_helper_completion_fn)(ring_helper_op_data_t *op_data,
                                          int res, unsigned flags);

// Embed this as a member of your per-op state struct and recover the outer
// struct in the completion with container_of.
struct ring_helper_op_data_t {
  ring_helper_completion_fn completion_fn;
  protothread_state *pt;  // used by ring_helper_resume_pt; may be nullptr
};

bool ring_helper_init(void);
void ring_helper_shutdown(void);

// One event-loop iteration:
//   1. run every completion currently available
//   2. submit everything queued (including ops queued BY those completions)
//   3. if block is true and step 1 found nothing, sleep until a CQE arrives
void ring_helper_tick(bool block);

// Returns an SQE, making room first if the submission queue is full.
// nullptr only on a real failure (already logged).
struct io_uring_sqe *ring_helper_get_sqe(void);

// Queue an op in one line:
//
//   ring_helper_queue(accept, &a->op, fd, addr, &addrlen, 0)
//
// `kind` is the io_uring_prep_ suffix (accept, recv, send, openat, ...).
// Evaluates to true if queued; on false nothing was prepared and the caller
// still owns op_data. Does not submit: ring_helper_tick does.
#define ring_helper_queue(kind, op_data, ...)                                  \
  ({                                                                           \
    struct io_uring_sqe *_sqe = ring_helper_get_sqe();                         \
    if (_sqe) {                                                                \
      io_uring_prep_##kind(_sqe __VA_OPT__(, ) __VA_ARGS__);                   \
      io_uring_sqe_set_data(_sqe, (op_data));                                  \
    }                                                                          \
    _sqe != nullptr;                                                           \
  })

// Opt-in stock completion for simple one-shot ops with nothing to clean up:
// sets pt->error (0 or an errno value) and resumes the protothread.
// Ops that own state (send progress, accept slots) write their own.
void ring_helper_resume_pt(ring_helper_op_data_t *op_data, int res,
                           unsigned flags);