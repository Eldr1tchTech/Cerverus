#pragma once

#include "defines.h"

typedef char *string;

string string_create(char *str);

int string_compare(const string s1, const string s2);

size_t raw_string_length(const char *str);