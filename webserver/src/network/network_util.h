#pragma once

#include "defines.h"

sds *parse_URI(char *URI);

lit_t content_type_val_helper(sds ext);