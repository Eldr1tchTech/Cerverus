#pragma once

#include "defines.h"

typedef struct sv_t {
  const char *str;
  size_t len;
} sv_t;

sv_t *sv_create(char *str, size_t len);

void sv_destroy(sv_t *sv);

static inline bool sv_eq(sv_t sv, const char *str, size_t len);

#define sv_eq_lit(sv, lit) sv_eq(sv, "" lit, sizeof(lit) - 1)