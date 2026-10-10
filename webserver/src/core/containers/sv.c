#include "sv.h"
#include "core/memory/cmem.h"
#include <string.h>

sv_t *sv_create(char *str, size_t len) {
  sv_t *new_sv = cmem_alloc(sizeof(sv_t));

  new_sv->str = str;
  new_sv->len = len;

  return new_sv;
}

void sv_destroy(sv_t *sv) {
  sv->str = 0;
  sv->len = 0;
  cmem_free(sv);
}

static inline bool sv_eq(sv_t sv, const char *str, size_t len) {
  return (sv.len == len) && memcmp(sv.str, str, len) == 0;
}
