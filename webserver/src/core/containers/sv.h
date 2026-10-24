#pragma once

#include "defines.h"

typedef struct sv_t {
  const char *ptr;
  size_t len;
} sv_t;

sv_t *sv_create(char *str, size_t len);

void sv_destroy(sv_t *sv);

static inline bool sv_eq(sv_t sv, const char *str, size_t len);
static inline bool sv_eq_ci(sv_t a, sv_t b) { return a.len == b.len && strncasecmp(a.ptr, b.ptr, a.len) == 0; }

#define sv_eq_lit(sv, lit) sv_eq(sv, "" lit, sizeof(lit) - 1)