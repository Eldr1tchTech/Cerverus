#include "doubly_linked_list.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"

// TODO: Add an option to instantiate a pool of nodes, to allow for an increase
// in performance when creating and destroying nodes.

doubly_linked_list_node *doubly_linked_list_node_create(size_t stride) {
  doubly_linked_list_node *new_node =
      cmem_alloc(sizeof(doubly_linked_list_node) + stride);

  new_node->prev = nullptr;
  new_node->next = nullptr;

  return new_node;
}

// TODO: Check this again.
void doubly_linked_list_node_destroy(doubly_linked_list_node *node) {
  cmem_free(node);
}

doubly_linked_list *doubly_linked_list_create(size_t stride) {
  doubly_linked_list *new_dll = cmem_alloc(sizeof(doubly_linked_list));

  new_dll->stride = stride;
  new_dll->length = 0;
  new_dll->head = nullptr;
  new_dll->tail = nullptr;

  return new_dll;
}

void doubly_linked_list_destroy(doubly_linked_list *dll) {
  doubly_linked_list_node *curr_node = dll->head;
  doubly_linked_list_node *temp;
  while (curr_node != nullptr) {
    temp = curr_node->next;
    doubly_linked_list_node_destroy(curr_node);
    curr_node = temp;
  }
  cmem_free(dll);
}

// Access
doubly_linked_list_node *doubly_linked_list_get(doubly_linked_list *dll,
                                                int index) {
  if (index >= dll->length || index <= -dll->length - 1) {
    LOG_ERROR("doubly_linked_list_get - index out of bounds.");
    return nullptr;
  }

  if (index == 0) {
    return dll->head;
  } else if (index == -1) {
    return dll->tail;
  }

  doubly_linked_list_node *curr_node;
  if (index < 0) {
    curr_node = dll->tail;
    for (size_t i = -1; i < index; i--) {
      curr_node = curr_node->prev;
    }
  } else {
    curr_node = dll->head;
    for (size_t i = 0; i < index; i++) {
      curr_node = curr_node->next;
    }
  }
  return curr_node;
}

// Insert
doubly_linked_list_node *doubly_linked_list_push_front(doubly_linked_list *dll,
                                                       void *data) {
  doubly_linked_list_node *new_node =
      doubly_linked_list_node_create(dll->stride);
  cmem_mcpy(new_node->data, data, dll->stride);

  dll->head->prev = new_node;
  new_node->next = dll->head;
  dll->head = new_node;
  new_node->prev = nullptr;

  dll->length++;

  return new_node;
}

doubly_linked_list_node *doubly_linked_list_push_back(doubly_linked_list *dll,
                                                      void *data) {

  doubly_linked_list_node *new_node =
      doubly_linked_list_node_create(dll->stride);
  cmem_mcpy(new_node->data, data, dll->stride);

  dll->tail->next = new_node;
  new_node->prev = dll->tail;
  dll->tail = new_node;
  new_node->next = nullptr;

  dll->length++;

  return new_node;
}

// Remove
void doubly_linked_list_pop_head(doubly_linked_list *dll, void *data) {
  if (dll->length == 0) {
    LOG_ERROR("doubly_linked_list_pop_front - length is zero.");
    return;
  }

  if (data != nullptr)
    cmem_mcpy(data, dll->head->data, dll->stride);

  if (dll->length > 1) {
    dll->head = dll->head->next;
    doubly_linked_list_node_destroy(dll->head->prev);
    dll->head->prev = nullptr;
  } else {
    doubly_linked_list_node_destroy(dll->head);
    dll->head = nullptr;
    dll->tail = nullptr;
  }

  dll->length--;
}

void doubly_linked_list_pop_tail(doubly_linked_list *dll, void *data) {
  if (dll->length == 0) {
    LOG_ERROR("doubly_linked_list_pop_front - length is zero.");
    return;
  }

  if (data != nullptr)
    cmem_mcpy(data, dll->tail->data, dll->stride);

  if (dll->length > 1) {
    dll->tail = dll->tail->prev;
    doubly_linked_list_node_destroy(dll->tail->next);
    dll->tail->next = nullptr;
  } else {
    doubly_linked_list_node_destroy(dll->tail);
    dll->head = nullptr;
    dll->tail = nullptr;
  }

  dll->length--;
}

void doubly_linked_list_move_to_front(doubly_linked_list *dll,
                                      doubly_linked_list_node *node) {
  if (dll->head == node)
    return;

  node->prev->next = node->next;
  if (node->next) {
    node->next->prev = node->prev;
  };
  node->next = dll->head;
  dll->head->prev = node;
  dll->head = node;
  node->prev = nullptr;
}
