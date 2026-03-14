#include "network_util.h"

#include "core/util/util.h"
#include "core/memory/cmem.h"

darray* parse_URI(char* URI) {
    darray* new_darr = darray_create(strchrc(URI, '/'), sizeof(char *));

    URI++; // Skip the initial /
    route_segment* segment;

    int count = new_darr->size;
    for (int i = 0; i < count; i++)
    {
        darray_add(new_darr, &segment);
        segment = (route_segment*)darray_get(new_darr, i);

        if (URI[0] == ':')
        {
            segment->is_dynamic = true;
            URI++;
        } else {
            segment->is_dynamic = false;
        }
        
        int char_index = strchri(URI, '/');
        segment->path_segment = cmem_alloc(memory_tag_string, (char_index + 1) * sizeof(char));

        cmem_mcpy(segment->path_segment, URI, (char_index + 1) * sizeof(char));

        URI += (char_index + 1);
    }

    return new_darr;
}