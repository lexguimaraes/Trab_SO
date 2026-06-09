#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

void memory_init(MemoryManager *memory, int total_size)
{
    memory->blocks = NULL;
    memory->count = 0;
    memory->capacity = 0;

    memory->blocks = malloc(sizeof(MemoryBlock));
    if (memory->blocks == NULL) {
        return;
    }

    memory->blocks[0] = (MemoryBlock){
        .base = 0,
        .size = total_size,
        .pid = -1,
        .free = 1,
    };
    memory->count = 1;
    memory->capacity = 1;
}

void memory_destroy(MemoryManager *memory)
{
    free(memory->blocks);
    memory->blocks = NULL;
    memory->count = 0;
    memory->capacity = 0;
}

static int memory_grow(MemoryManager *memory)
{
    size_t next_capacity = memory->capacity == 0 ? 4 : memory->capacity * 2;
    MemoryBlock *next_blocks = realloc(memory->blocks, next_capacity * sizeof(MemoryBlock));
    if (next_blocks == NULL) {
        return -1;
    }

    memory->blocks = next_blocks;
    memory->capacity = next_capacity;
    return 0;
}

static int memory_insert_block(MemoryManager *memory, size_t index, MemoryBlock block)
{
    if (memory->count == memory->capacity && memory_grow(memory) != 0) {
        return -1;
    }

    for (size_t i = memory->count; i > index; i--) {
        memory->blocks[i] = memory->blocks[i - 1];
    }

    memory->blocks[index] = block;
    memory->count++;
    return 0;
}

int memory_allocate(MemoryManager *memory, int pid, int size)
{
    for (size_t i = 0; i < memory->count; i++) {
        MemoryBlock *block = &memory->blocks[i];
        if (!block->free || block->size < size) {
            continue;
        }

        int base = block->base;
        if (block->size == size) {
            block->pid = pid;
            block->free = 0;
            return base;
        }

        MemoryBlock remaining = {
            .base = block->base + size,
            .size = block->size - size,
            .pid = -1,
            .free = 1,
        };

        block->size = size;
        block->pid = pid;
        block->free = 0;

        if (memory_insert_block(memory, i + 1, remaining) != 0) {
            block->size += remaining.size;
            block->pid = -1;
            block->free = 1;
            return -1;
        }

        return base;
    }

    return -1;
}

static void memory_coalesce(MemoryManager *memory)
{
    size_t i = 0;
    while (i + 1 < memory->count) {
        MemoryBlock *current = &memory->blocks[i];
        MemoryBlock *next = &memory->blocks[i + 1];

        if (current->free && next->free) {
            current->size += next->size;
            for (size_t j = i + 1; j + 1 < memory->count; j++) {
                memory->blocks[j] = memory->blocks[j + 1];
            }
            memory->count--;
            continue;
        }

        i++;
    }
}

void memory_release(MemoryManager *memory, int pid)
{
    for (size_t i = 0; i < memory->count; i++) {
        MemoryBlock *block = &memory->blocks[i];
        if (!block->free && block->pid == pid) {
            block->pid = -1;
            block->free = 1;
            memory_coalesce(memory);
            return;
        }
    }
}

void memory_print(const MemoryManager *memory)
{
    printf("Memoria:");
    for (size_t i = 0; i < memory->count; i++) {
        const MemoryBlock *block = &memory->blocks[i];
        if (block->free) {
            printf(" [%d,%d livre]", block->base, block->size);
        } else {
            printf(" [%d,%d P%d]", block->base, block->size, block->pid);
        }
    }
    printf("\n");
}
