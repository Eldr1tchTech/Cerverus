#pragma once

#include "defines.h"

typedef struct doubly_linked_list_node
{
    struct doubly_linked_list_node* prev;
    struct doubly_linked_list_node* next;
    char data[];
} doubly_linked_list_node;

typedef struct doubly_linked_list
{
    size_t length;
    size_t stride;
    doubly_linked_list_node* head;
    doubly_linked_list_node* tail;
} doubly_linked_list;

doubly_linked_list* doubly_linked_list_create(size_t stride);
void doubly_linked_list_destroy(doubly_linked_list* dll);

// Access
void* doubly_linked_list_get(doubly_linked_list* dll, size_t index);

// Insert
void doubly_linked_list_push_front(doubly_linked_list* dll, void* data);
void doubly_linked_list_push_back(doubly_linked_list* dll, void* data);
void doubly_linked_list_insert_at(doubly_linked_list* dll, size_t index, void* data);

// Remove
// Caller needs to allocate memory into which the data should be copied.
void doubly_linked_list_pop_front(doubly_linked_list* dll, void* data);
void doubly_linked_list_pop_back(doubly_linked_list* dll, void* data);
void doubly_linked_list_pop_at(doubly_linked_list* dll, size_t index, void* data);
void doubly_linked_list_pop_node(doubly_linked_list* dll, doubly_linked_list_node* node);

// Move
void doubly_linked_list_move_to_front(doubly_linked_list* dll, doubly_linked_list_node* node);