#include "queue.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"

queue *queue_create(int stride) {
  queue *q = cmem_alloc(sizeof(queue));
  q->stride = stride;
  q->root = nullptr;
  q->head = nullptr;
}

void queue_destroy(queue *q) {
  queue_clear(q);
  cmem_free(q);
  q = 0;
}

// NOTE: This copies the memory into memory allocated by the queue.
// It DOES NOT take ownership of the memory of item.
void enqueue(queue *q, void *item) {
  node *new_node = cmem_alloc(sizeof(node));
  new_node->data = cmem_alloc(q->stride);
  if (!q->head) {
    q->root = new_node;
    q->head = new_node;
  } else {
    q->head->next = new_node;
    q->head = new_node;
  }
  new_node->next = nullptr;
  cmem_mcpy(q->head->data, item, q->stride);
}

// NOTE: This transfers ownership of the memory to the caller.
void *dequeue(queue *q) {
  if (!q->head) {
    LOG_DEBUG(
        "dequeue - Attempted to dequeue an empty queue. Returning nullptr.");
    return nullptr;
  }

  node *node = q->root;
  q->root = q->root->next;
  if (!q->root) {
    q->head = nullptr;
  }
  void *return_dat = node->data;
  cmem_free(node);
  return return_dat;
}

void queue_clear(queue *q) {
  while (q->root) {
    cmem_free(dequeue(q));
  }
}
