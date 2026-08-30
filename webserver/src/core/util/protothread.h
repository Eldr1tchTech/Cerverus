#pragma once

// Protothreads via computed goto (GNU/Clang extension: `&&label`, `goto *ptr`).

typedef struct protothread_state {
  void *resume_label;
  void (*self)(void *frame);
} protothread_state;

// Aliasing
#define PT_CONCAT_(a, b) a##b
#define PT_CONCAT(a, b) PT_CONCAT_(a, b)
#define PT_LABEL_NAME PT_CONCAT(pt_resume_, __LINE__)

// Dispatches to the saved suspend point, or falls through to the top on
// a fresh call.
#define PT_BEGIN(pt_ptr, fn)                                                   \
  do {                                                                         \
    (pt_ptr)->self = (fn);                                                     \
    if ((pt_ptr)->resume_label)                                                \
      goto *(pt_ptr)->resume_label;                                            \
  } while (0)

// Submits async work, records where to resume, and returns control to the
// caller (the event loop). Re-entry via a fresh function call lands exactly
// at the label below, past the case-equivalent.
#define PT_WAIT(pt_ptr, submit_expr)                                           \
  do {                                                                         \
    (pt_ptr)->resume_label = &&PT_LABEL_NAME;                                  \
    submit_expr;                                                               \
    return;                                                                    \
  PT_LABEL_NAME:;                                                              \
  } while (0)

#define PT_END(pt_ptr) ((pt_ptr)->resume_label = nullptr)