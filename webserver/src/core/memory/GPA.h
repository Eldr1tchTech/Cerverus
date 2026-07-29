#include "defines.h"

#include "core/containers/doubly_linked_list.h"

typedef struct GPA_allocation_header {
  size_t size;
} GPA_allocation_header;

typedef struct free_list_node {
  size_t size;
  size_t offset;
} free_list_node;

typedef struct GPA {
  doubly_linked_list *free_list;
  size_t size;
  char data[];
} GPA;

GPA *GPA_create(size_t size);
void GPA_destroy(GPA *gpa);

void *GPA_alloc(GPA *gpa, size_t size);
bool GPA_free(GPA *gpa, void *ptr);
