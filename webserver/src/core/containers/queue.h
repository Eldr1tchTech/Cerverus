#pragma once

typedef struct node
{
    void* data;
    void* next;
} node;

typedef struct queue
{
    int stride;
    node* head;
    node* root;
} queue;

queue* queue_create(int stride);
void queue_destroy(queue* q);

void enqueue(queue* q, void* item);
void* dequeue(queue* q);

void queue_clear(queue* q);