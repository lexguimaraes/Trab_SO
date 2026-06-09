#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>

typedef enum {
    PROCESS_REAL_TIME = 0,
    PROCESS_USER = 1
} ProcessPriority;

typedef enum {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED_IO,
    PROCESS_FINISHED
} ProcessState;

typedef enum {
    PHASE_CPU1,
    PHASE_IO,
    PHASE_CPU2,
    PHASE_DONE
} ProcessPhase;

typedef struct {
    int id;
    ProcessPriority priority;
    int arrival_time;
    int cpu1_time;
    int io_time;
    int cpu2_time;
    int memory_mb;
    int requested_disks;

    ProcessState state;
    ProcessPhase phase;
    int remaining_time;
    int quantum_remaining;
    int feedback_level;
    int memory_base;
} Process;

typedef struct {
    Process *items;
    size_t count;
    size_t capacity;
} ProcessList;

void process_list_init(ProcessList *list);
void process_list_destroy(ProcessList *list);
int process_list_load(ProcessList *list, const char *path);

const char *process_state_name(ProcessState state);
const char *process_phase_name(ProcessPhase phase);

#endif
