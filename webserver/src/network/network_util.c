#include "network_util.h"

#include "core/util/util.h"
#include "core/util/logger.h"
#include "core/memory/cmem.h"

#include <string.h>

darray* parse_URI(char* URI) {
    darray* new_darr = darray_create(strchrc(URI, '/'), sizeof(route_segment));

    URI++; // Skip the initial /

    int count = new_darr->size;
    for (int i = 0; i < count; i++)
    {
        route_segment segment = {0};

        if (URI[0] == ':')
        {
            segment.is_dynamic = true;
            URI++;
        }

        int char_index = strchri(URI, '/');
        int seg_len = (char_index == -1) ? (int)strlen(URI) : char_index;

        segment.path_segment = cmem_alloc(memory_tag_string, (seg_len + 1) * sizeof(char));
        cmem_mcpy(segment.path_segment, URI, seg_len);
        // +1 byte is already zeroed by cmem_alloc, so null terminator is handled

        URI += seg_len + (char_index == -1 ? 0 : 1);

        darray_add(new_darr, &segment);
    }

    return new_darr;
}

char *content_type_val_helper(const char *ext)
{
    if (ext)
    {
        if (strcmp(ext + 1, "html") == 0)
        {
            return "text/html";
        }
        else if (strcmp(ext + 1, "css") == 0)
        {
            return "text/css";
        }
        else if (strcmp(ext + 1, "jpg") == 0 || strcmp(ext + 1, "jpeg") == 0)
        {
            return "image/jpeg";
        }
        else if (strcmp(ext + 1, "png") == 0)
        {
            return "image/png";
        }
        else if (strcmp(ext + 1, "gif") == 0)
        {
            return "image/gif";
        }
        else if (strcmp(ext + 1, "webp") == 0)
        {
            return "image/webp";
        }
        else if (strcmp(ext + 1, "svg") == 0)
        {
            return "image/svg+xml";
        }
        else if (strcmp(ext + 1, "ico") == 0)
        {
            return "image/x-icon";
        }
        else
        {
            LOG_ERROR("content_type_val_helper - Currently unsuported file extension: %s. Returning null.", ext);
            return NULL;
        }
    }
    LOG_ERROR("content_type_val_helper - Please provide a valid char* for ext.");
    return NULL;
}

void darray_destroy_string_helper(darray* darr) {
    char** darr_data = darr->data;
    for (size_t i = 0; i < darr->length; i++)
    {
        cmem_free(memory_tag_string, darr_data[i]);
    }
    darray_destroy(darr);
}