#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

typedef struct {
    int base;
    int size;
    int pid;
    int free;
} MemoryBlock;

typedef struct {
    MemoryBlock *blocks;
    size_t count;
    size_t capacity;
} MemoryManager;

void memory_init(MemoryManager *memory, int total_size);
void memory_destroy(MemoryManager *memory);
int memory_allocate(MemoryManager *memory, int pid, int size);
void memory_release(MemoryManager *memory, int pid);
void memory_print(const MemoryManager *memory);

#endif
