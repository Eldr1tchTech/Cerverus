#pragma once

#include "defines.h"

typedef struct sv_t {
  const char *str;
  size_t len;
} sv_t;

sv_t *sv_create(char *str, size_t len);

void sv_destroy(sv_t *sv);