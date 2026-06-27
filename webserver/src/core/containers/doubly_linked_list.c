#include "doubly_linked_list.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"

// TODO: Add an option to instantiate a pool of nodes, to allow for an increase in performance when creating and destroying nodes.

doubly_linked_list_node *doubly_linked_list_node_create(size_t stride)
{
    doubly_linked_list_node *new_node = cmem_alloc(memory_tag_doubly_linked_list, sizeof(doubly_linked_list_node) + stride);

    new_node->prev = NULL;
    new_node->next = NULL;

    return new_node;
}

void doubly_linked_list_node_destroy(doubly_linked_list_node *node)
{
    cmem_free(memory_tag_doubly_linked_list, node);
}

doubly_linked_list *doubly_linked_list_create(size_t stride)
{
    doubly_linked_list *new_dll = cmem_alloc(memory_tag_doubly_linked_list, sizeof(doubly_linked_list));

    new_dll->stride = stride;
    new_dll->length = 0;
    new_dll->head = NULL;
    new_dll->tail = NULL;

    return new_dll;
}

void doubly_linked_list_destroy(doubly_linked_list *dll)
{
    doubly_linked_list_node *curr_node = dll->head;
    doubly_linked_list_node *temp;
    while (curr_node != NULL)
    {
        temp = curr_node->next;
        doubly_linked_list_node_destroy(curr_node);
        curr_node = temp;
    }
    cmem_free(memory_tag_doubly_linked_list, dll);
}

// Access
// TODO: Make it calculate whether to approach from the front or the back.
doubly_linked_list_node *doubly_linked_list_get_node(doubly_linked_list *dll, size_t index)
{
    if (index >= dll->length)
    {
        LOG_ERROR("doubly_linked_list_get_node - index is greater than or equal to length.");
        return NULL;
    }

    doubly_linked_list_node *curr_node = dll->head;
    for (size_t i = 0; i < index; i++)
    {
        curr_node = curr_node->next;
    }
    return curr_node;
}

void *doubly_linked_list_get(doubly_linked_list *dll, size_t index)
{
    return doubly_linked_list_get_node(dll, index)->data;
}

// Insert
void doubly_linked_list_push_front(doubly_linked_list *dll, void *data)
{
    doubly_linked_list_node *new_node = doubly_linked_list_node_create(dll->stride);

    cmem_mcpy(new_node->data, data, dll->stride);
    new_node->next = dll->head;
    if (dll->length != 0) dll->head->prev = new_node;
    dll->head = new_node;
    dll->length++;

    if (dll->length == 1)
        dll->tail = new_node;
}

void doubly_linked_list_push_back(doubly_linked_list *dll, void *data)
{
    doubly_linked_list_node *new_node = doubly_linked_list_node_create(dll->stride);

    cmem_mcpy(new_node->data, data, dll->stride);
    new_node->prev = dll->tail;
    if (dll->length != 0) dll->tail->next = new_node;
    dll->tail = new_node;
    dll->length++;

    if (dll->length == 1)
        dll->head = new_node;
}

void doubly_linked_list_insert_at(doubly_linked_list *dll, size_t index, void *data)
{
    if (index >= dll->length)
    {
        LOG_ERROR("doubly_linked_list_insert_at - index out of bounds.");
        return;
    }
    else if (dll->length == 0)
    {
        doubly_linked_list_push_front(dll, data);
        return;
    }
    else if (index == dll->length - 1)
    {
        doubly_linked_list_push_back(dll, data);
        return;
    }

    doubly_linked_list_node *new_node = doubly_linked_list_node_create(dll->stride);
    doubly_linked_list_node *prev_node = doubly_linked_list_get_node(dll, index - 1);
    doubly_linked_list_node *next_node = prev_node->next;

    cmem_mcpy(new_node->data, data, dll->stride);
    new_node->prev = prev_node;
    new_node->next = next_node;

    prev_node->next = new_node;
    next_node->prev = new_node;
}

// Remove
void doubly_linked_list_pop_front(doubly_linked_list *dll, void *data)
{
    if (dll->length == 0)
    {
        LOG_ERROR("doubly_linked_list_pop_front - length is zero.");
        return;
    }

    if (data != NULL)
        cmem_mcpy(data, dll->head->data, dll->stride);

    if (dll->length > 1)
    {
        dll->head = dll->head->next;
        doubly_linked_list_node_destroy(dll->head->prev);
        dll->head->prev = NULL;
    }
    else
    {
        doubly_linked_list_node_destroy(dll->head);
        dll->head = NULL;
        dll->tail = NULL;
    }

    dll->length--;
}

void doubly_linked_list_pop_back(doubly_linked_list *dll, void *data)
{
    if (dll->length == 0)
    {
        LOG_ERROR("doubly_linked_list_pop_front - length is zero.");
        return;
    }

    if (data != NULL)
        cmem_mcpy(data, dll->tail->data, dll->stride);

    if (dll->length > 1)
    {
        dll->tail = dll->tail->prev;
        doubly_linked_list_node_destroy(dll->tail->next);
        dll->tail->next = NULL;
    }
    else
    {
        // NOTE: does it make a performance difference whther I do the tail or the head here?
        // Whichever is first in the struct right? since the other might be over a cache boundary,
        // and cause another cache-line fetch?
        doubly_linked_list_node_destroy(dll->tail);
        dll->head = NULL;
        dll->tail = NULL;
    }

    dll->length--;
}

void doubly_linked_list_pop_at(doubly_linked_list *dll, size_t index, void *data)
{
    if (dll->length == 0)
    {
        LOG_ERROR("doubly_linked_list_remove_at - length is zero.");
        return;
    }
    else if (index >= dll->length)
    {
        LOG_ERROR("doubly_linked_list_remove_at - index out of bounds.");
        return;
    }
    else if (index == 0)
    {
        doubly_linked_list_pop_front(dll, data);
        return;
    }
    else if (index == dll->length - 1)
    {
        doubly_linked_list_pop_back(dll, data);
        return;
    }

    doubly_linked_list_node *node = doubly_linked_list_get_node(dll, index);

    if (data != NULL)
        cmem_mcpy(data, node->data, dll->stride);

    node->prev->next = node->next;
    node->next->prev = node->prev;
    doubly_linked_list_node_destroy(node);

    dll->length--;
}

void doubly_linked_list_pop_node(doubly_linked_list* dll, doubly_linked_list_node* node) {
    if (node == dll->head) doubly_linked_list_pop_front(dll, NULL);
    if (node == dll->tail) doubly_linked_list_pop_back(dll, NULL);
    
    node->prev->next = node->next;
    node->next->prev = node->prev;
    doubly_linked_list_node_destroy(node);

    dll->length--;
}

void doubly_linked_list_push_node_to_front(doubly_linked_list* dll, doubly_linked_list_node* node) {
    doubly_linked_list_push_front(dll, node->data);
    doubly_linked_list_pop_node(dll, node);
}