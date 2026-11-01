#pragma once

#include "defines.h"
#include <strings.h>

typedef struct sv_t {
  char *ptr;
  size_t len;
} sv_t;

static inline int svcmplen(const sv_t sv, lit_t lit, size_t litlen) {
  return _strcmp(sv.ptr, sv.len, lit, litlen);
}

#define svcmplit(sv, lit) svcmplen((sv), "" lit, litlen(lit))

sv_t *sv_create(char *str, size_t len);

void sv_destroy(sv_t *sv);

static inline int sv_cmp(sv_t a, sv_t b) {
  return _strcmp(a.ptr, a.len, a.ptr, a.len);
}

static inline bool sv_eq(sv_t sv, const char *str, size_t len) {return sv.len == len && _strcmp(sv.ptr, sv.len, str, len) == 0;}
static inline bool sv_eq_ci(sv_t a, sv_t b) { return a.len == b.len && strncasecmp(a.ptr, b.ptr, a.len) == 0; }

#define sv_eq_lit(sv, lit) sv_eq(sv, "" lit, sizeof(lit) - 1)