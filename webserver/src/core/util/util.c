#include "util.h"

#include "core/util/logger.h"
#include "core/memory/cmem.h"

#include <stdarg.h>
#include <stdio.h>

char *asprintf_cerv(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (needed < 0)
    {
        LOG_DEBUG("asprintf - Tried to return value of length 0.");
    }

    char *str = cmem_alloc(memory_tag_string, needed + 1);
    if (str)
    {
        vsnprintf(str, needed + 1, fmt, args);
    }

    va_end(args);
    return str;
}

// STRing CHaRacter Count
int strchrc(char *str, char c)
{
    int count = 0;
    while (*str != '\0')
    {
        if (*str == c)
        {
            count++;
        }
        str++;
    }
    return count;
}

// STRing CHaRacter Index
int strchri(char *str, char c)
{
    int i = 0;
    while (*str != '\0')
    {
        if (*str == c)
        {
            return i;
        }
        else
        {
            i++;
        }
        str++;
    }
    return -1;
}