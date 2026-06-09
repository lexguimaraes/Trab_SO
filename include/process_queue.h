#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H

#include "process.h"

#include <stddef.h>

typedef struct {
    Process **items;
    size_t count;
    size_t capacity;
} ProcessQueue;

void queue_init(ProcessQueue *queue);
void queue_destroy(ProcessQueue *queue);
int queue_push(ProcessQueue *queue, Process *process);
Process *queue_pop(ProcessQueue *queue);
int queue_empty(const ProcessQueue *queue);

#endif
