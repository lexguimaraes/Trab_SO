#include "process_queue.h"

#include <stdlib.h>

void queue_init(ProcessQueue *queue)
{
    queue->items = NULL;
    queue->count = 0;
    queue->capacity = 0;
}

void queue_destroy(ProcessQueue *queue)
{
    free(queue->items);
    queue->items = NULL;
    queue->count = 0;
    queue->capacity = 0;
}

static int queue_grow(ProcessQueue *queue)
{
    size_t next_capacity = queue->capacity == 0 ? 8 : queue->capacity * 2;
    Process **next_items = realloc(queue->items, next_capacity * sizeof(Process *));
    if (next_items == NULL) {
        return -1;
    }

    queue->items = next_items;
    queue->capacity = next_capacity;
    return 0;
}

int queue_push(ProcessQueue *queue, Process *process)
{
    if (queue->count == queue->capacity && queue_grow(queue) != 0) {
        return -1;
    }

    queue->items[queue->count] = process;
    queue->count++;
    return 0;
}

Process *queue_pop(ProcessQueue *queue)
{
    if (queue->count == 0) {
        return NULL;
    }

    Process *process = queue->items[0];
    for (size_t i = 1; i < queue->count; i++) {
        queue->items[i - 1] = queue->items[i];
    }
    queue->count--;
    return process;
}

int queue_empty(const ProcessQueue *queue)
{
    return queue->count == 0;
}
