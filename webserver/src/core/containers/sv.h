#pragma once

#include "defines.h"

typedef struct sv_t {
  const char *ptr;
  size_t len;
} sv_t;

static inline int svcmplitlen(const sv_t sv, lit_t lit, size_t litlen) {
  size_t len = sv.len;
  size_t minlen = len < litlen ? len : litlen;
  int cmp = memcmp(sv.ptr, lit, minlen);
  if (cmp == 0)
    return len > litlen ? 1 : (len < litlen ? -1 : 0);
  return cmp;
}

#define svcmplit(sv, lit) svcmplitlen((sv), "" lit, litlen(lit))

sv_t *sv_create(char *str, size_t len);

void sv_destroy(sv_t *sv);

static inline bool sv_eq(sv_t sv, const char *str, size_t len);
static inline bool sv_eq_ci(sv_t a, sv_t b) { return a.len == b.len && strncasecmp(a.ptr, b.ptr, a.len) == 0; }

#define sv_eq_lit(sv, lit) sv_eq(sv, "" lit, sizeof(lit) - 1)