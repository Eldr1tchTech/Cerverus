#include "queue.h"

#include "memory/cmem.h"
#include "core/util/logger.h"

queue* queue_create(int stride) {
    queue* q = cmem_alloc(memory_tag_queue, sizeof(queue));
    q->stride = stride;
    q->root = NULL;
    q->head = NULL;
}

void queue_destroy(queue* q) {
    queue_clear(q);
    cmem_free(memory_tag_queue, q);
    q = 0;
}

// NOTE: This copies the memory into memory allocated by the queue.
// It DOES NOT take ownership of the memory of item.
void enqueue(queue* q, void* item) {
    node* new_node = cmem_alloc(memory_tag_queue, sizeof(node));
    new_node->data = cmem_alloc(memory_tag_queue, q->stride);
    if (!q->head)
    {
        q->root = new_node;
        q->head = new_node;
    } else {
        q->head->next = new_node;
        q->head = new_node;
    }
    new_node->next = NULL;
    cmem_mcpy(q->head->data, item, q->stride);
}

// NOTE: This transfers ownership of the memory to the caller.
void* dequeue(queue* q) {
    if (!q->head)
    {
        LOG_DEBUG("dequeue - Attempted to dequeue an empty queue. Returning NULL.");
        return NULL;
    }
    
    node* node = q->root;
    q->root = q->root->next;
    if (!q->root)
    {
        q->head = NULL;
    }
    void* return_dat = node->data;
    cmem_free(memory_tag_queue, node);
    return return_dat;
}

void queue_clear(queue* q) {
    while (q->root)
    {
        cmem_free(memory_tag_queue, dequeue(q));
    }
}