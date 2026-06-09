#include "process.h"

#include "simulator.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void process_list_init(ProcessList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void process_list_destroy(ProcessList *list)
{
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static int process_list_grow(ProcessList *list)
{
    size_t next_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
    Process *next_items = realloc(list->items, next_capacity * sizeof(Process));
    if (next_items == NULL) {
        return -1;
    }

    list->items = next_items;
    list->capacity = next_capacity;
    return 0;
}

static int parse_priority(int value, ProcessPriority *priority)
{
    if (value == 0) {
        *priority = PROCESS_REAL_TIME;
        return 0;
    }
    if (value == 1) {
        *priority = PROCESS_USER;
        return 0;
    }
    return -1;
}

static int process_list_append(ProcessList *list, Process process)
{
    if (list->count == list->capacity && process_list_grow(list) != 0) {
        return -1;
    }

    list->items[list->count] = process;
    list->count++;
    return 0;
}

static int process_compare_arrival(const void *left, const void *right)
{
    const Process *a = left;
    const Process *b = right;

    if (a->arrival_time != b->arrival_time) {
        return a->arrival_time - b->arrival_time;
    }
    return a->id - b->id;
}

int process_list_load(ProcessList *list, const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "Erro ao abrir '%s': %s\n", path, strerror(errno));
        return -1;
    }

    char line[256];
    int line_number = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;

        char *cursor = line;
        while (*cursor == ' ' || *cursor == '\t') {
            cursor++;
        }
        if (*cursor == '\0' || *cursor == '\n' || *cursor == '#') {
            continue;
        }

        int id;
        int priority_value;
        int arrival_time;
        int cpu1_time;
        int io_time;
        int cpu2_time;
        int memory_mb;
        int requested_disks;

        int fields = sscanf(cursor, "%d %d %d %d %d %d %d %d",
                            &id,
                            &priority_value,
                            &arrival_time,
                            &cpu1_time,
                            &io_time,
                            &cpu2_time,
                            &memory_mb,
                            &requested_disks);
        if (fields != 8) {
            fprintf(stderr, "Linha %d invalida: esperado 8 campos.\n", line_number);
            fclose(file);
            return -1;
        }

        ProcessPriority priority;
        if (parse_priority(priority_value, &priority) != 0) {
            fprintf(stderr, "Linha %d invalida: prioridade deve ser 0 ou 1.\n", line_number);
            fclose(file);
            return -1;
        }

        if (arrival_time < 0 || cpu1_time < 0 || io_time < 0 || cpu2_time < 0 ||
            memory_mb <= 0 || requested_disks < 0 || requested_disks > SYSTEM_DISK_COUNT) {
            fprintf(stderr, "Linha %d invalida: valores fora do dominio.\n", line_number);
            fclose(file);
            return -1;
        }

        Process process = {
            .id = id,
            .priority = priority,
            .arrival_time = arrival_time,
            .cpu1_time = cpu1_time,
            .io_time = io_time,
            .cpu2_time = cpu2_time,
            .memory_mb = memory_mb,
            .requested_disks = requested_disks,
            .state = PROCESS_NEW,
            .phase = PHASE_CPU1,
            .remaining_time = cpu1_time,
            .quantum_remaining = USER_QUANTUM,
            .feedback_level = 0,
            .memory_base = -1,
        };

        if (process_list_append(list, process) != 0) {
            fprintf(stderr, "Sem memoria ao carregar processos.\n");
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    qsort(list->items, list->count, sizeof(Process), process_compare_arrival);
    return 0;
}

const char *process_state_name(ProcessState state)
{
    switch (state) {
    case PROCESS_NEW:
        return "NEW";
    case PROCESS_READY:
        return "READY";
    case PROCESS_RUNNING:
        return "RUNNING";
    case PROCESS_BLOCKED_IO:
        return "BLOCKED_IO";
    case PROCESS_FINISHED:
        return "FINISHED";
    }
    return "UNKNOWN";
}

const char *process_phase_name(ProcessPhase phase)
{
    switch (phase) {
    case PHASE_CPU1:
        return "CPU1";
    case PHASE_IO:
        return "IO";
    case PHASE_CPU2:
        return "CPU2";
    case PHASE_DONE:
        return "DONE";
    }
    return "UNKNOWN";
}
