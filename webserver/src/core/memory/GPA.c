#include "GPA.h"

#include "core/containers/doubly_linked_list.h"
#include "core/memory/cmem.h"

// WARN: DLL has had a major revamp to its API

GPA *GPA_create(size_t size) {
  GPA *gpa = cmem_alloc(sizeof(GPA) + size);

  gpa->size = size;
  gpa->free_list = doubly_linked_list_create(sizeof(free_list_node));

  free_list_node initial_node = {.size = size, .offset = 0};
  doubly_linked_list_push_back(gpa->free_list, &initial_node);

  return gpa;
}

void GPA_destroy(GPA *gpa) {
  doubly_linked_list_destroy(gpa->free_list);
  cmem_free(gpa);
}

// TODO: Add an iterator for the doubly_linked_list.
void *GPA_alloc(GPA *gpa, size_t size) {
  free_list_node *curr_fl_node;
  size_t adjusted_size = size + sizeof(GPA_allocation_header);

  doubly_linked_list_node *curr_dll_node = gpa->free_list->head;
  while (curr_dll_node != NULL) {
    curr_fl_node = (free_list_node *)curr_dll_node->data;
    if (curr_fl_node->size >= adjusted_size) {
      if (curr_fl_node->size > adjusted_size) {
        free_list_node new_fl_node = {
            .size = curr_fl_node->size - adjusted_size,
            .offset = curr_fl_node->offset + adjusted_size};
        // WARN: THIS WILL NOW WORK, FIX IT FIRST!
      } else {
      }
      GPA_allocation_header *new_alloc_header =
          (GPA_allocation_header *)gpa->data + curr_fl_node->offset;
      new_alloc_header->size = size;
      return new_alloc_header + sizeof(GPA_allocation_header);
    }
  }

  return NULL;
}

bool GPA_free(GPA *gpa, void *ptr) {
  if (gpa->data < ptr && ptr < gpa->data + gpa->size) {
    GPA_allocation_header *alloc_header = ptr - sizeof(GPA_allocation_header);
  }
  return false;
}
