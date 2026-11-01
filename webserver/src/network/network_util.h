#pragma once

#include "core/containers/sv.h"
#include "defines.h"

sds *parse_URI(char *URI);

lit_t content_type_val_helper(sv_t ext);