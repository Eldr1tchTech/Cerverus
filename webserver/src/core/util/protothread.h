#pragma once

#include "network/IO/async_io.h";

// NOTE: maybe move to async_io

// Protothreads via computed goto (GNU/Clang extension: `&&label`, `goto *ptr`).

// Aliasing
#define PT_CONCAT_(a, b) a##b
#define PT_CONCAT(a, b) PT_CONCAT_(a, b)
#define PT_LABEL_NAME PT_CONCAT(pt_resume_, __LINE__)

// Dispatches to the saved suspend point, or falls through to the top on
// a fresh call.
#define ASYNC_BEGIN(ctx, fn)                                                   \
  do {                                                                         \
    (ctx)->resume_point = (fn);                                                \
    if ((ctx)->resume_label)                                                   \
      goto *(ctx)->resume_label;                                               \
  } while (0)

// Submits async work, records where to resume, and returns control to the
// caller (the event loop). Re-entry via a fresh function call lands exactly
// at the label below, past the case-equivalent.
#define ASYNC(ctx, submit_expr)                                                \
  do {                                                                         \
    (ctx)->resume_label = &&ASYYNC_LABEL_NAME;                                 \
    submit_expr;                                                               \
    return;                                                                    \
  ASYNC_LABEL_NAME:;                                                           \
  } while (0)

#define ASYNC_END(ctx) ((ctx)->resume_label = nullptr)