#include "simulator.h"

#include "memory.h"
#include "process.h"
#include "process_queue.h"

#include <stdio.h>

typedef struct {
    Process *process;
} CpuSlot;

typedef struct {
    Process *process;
} DiskSlot;

typedef struct {
    ProcessQueue real_time_ready;
    ProcessQueue user_ready[3];
    ProcessQueue io_waiting;
    CpuSlot cpus[SYSTEM_CPU_COUNT];
    DiskSlot disks[SYSTEM_DISK_COUNT];
    MemoryManager memory;
    int finished_count;
} SingleThreadSystem;

static const char *priority_name(ProcessPriority priority)
{
    return priority == PROCESS_REAL_TIME ? "RT" : "USER";
}

static void print_rule(void)
{
    printf("-------------------------------------------------------------------------------\n");
}

static void print_event(int time, const char *type, int pid, const char *message)
{
    printf("  t=%03d | %-9s | P%-3d | %s\n", time, type, pid, message);
}

static void log_transition(int time, const Process *process, ProcessState previous)
{
    printf("  t=%03d | %-9s | P%-3d | %-10s -> %-10s | fase=%-4s restante=%d\n",
           time,
           "estado",
           process->id,
           process_state_name(previous),
           process_state_name(process->state),
           process_phase_name(process->phase),
           process->remaining_time);
}

static void set_state(int time, Process *process, ProcessState state)
{
    ProcessState previous = process->state;
    process->state = state;
    if (previous != state) {
        log_transition(time, process, previous);
    }
}

static int enqueue_ready(SingleThreadSystem *system, Process *process)
{
    if (process->priority == PROCESS_REAL_TIME) {
        return queue_push(&system->real_time_ready, process);
    }

    if (process->feedback_level < 0) {
        process->feedback_level = 0;
    } else if (process->feedback_level > 2) {
        process->feedback_level = 2;
    }
    return queue_push(&system->user_ready[process->feedback_level], process);
}

static Process *pop_next_ready(SingleThreadSystem *system)
{
    Process *process = queue_pop(&system->real_time_ready);
    if (process != NULL) {
        return process;
    }

    for (int level = 0; level < 3; level++) {
        process = queue_pop(&system->user_ready[level]);
        if (process != NULL) {
            return process;
        }
    }

    return NULL;
}

static void complete_process(SingleThreadSystem *system, int time, Process *process)
{
    process->phase = PHASE_DONE;
    process->remaining_time = 0;
    set_state(time, process, PROCESS_FINISHED);
    memory_release(&system->memory, process->id);
    system->finished_count++;
    print_event(time, "memoria", process->id, "finalizado; memoria liberada");
}

static int init_system(SingleThreadSystem *system)
{
    queue_init(&system->real_time_ready);
    for (int i = 0; i < 3; i++) {
        queue_init(&system->user_ready[i]);
    }
    queue_init(&system->io_waiting);

    for (int i = 0; i < SYSTEM_CPU_COUNT; i++) {
        system->cpus[i].process = NULL;
    }
    for (int i = 0; i < SYSTEM_DISK_COUNT; i++) {
        system->disks[i].process = NULL;
    }

    memory_init(&system->memory, SYSTEM_MEMORY_MB);
    system->finished_count = 0;
    return system->memory.blocks == NULL ? -1 : 0;
}

static void destroy_system(SingleThreadSystem *system)
{
    queue_destroy(&system->real_time_ready);
    for (int i = 0; i < 3; i++) {
        queue_destroy(&system->user_ready[i]);
    }
    queue_destroy(&system->io_waiting);
    memory_destroy(&system->memory);
}

static void admit_processes(SingleThreadSystem *system, ProcessList *processes, int time)
{
    for (size_t i = 0; i < processes->count; i++) {
        Process *process = &processes->items[i];
        if (process->state != PROCESS_NEW || process->arrival_time > time) {
            continue;
        }

        int base = memory_allocate(&system->memory, process->id, process->memory_mb);
        if (base < 0) {
            continue;
        }

        process->memory_base = base;
        process->feedback_level = 0;
        process->quantum_remaining = USER_QUANTUM;
        set_state(time, process, PROCESS_READY);
        printf("  t=%03d | %-9s | P%-3d | base=%-5d tamanho=%-5d MiB prioridade=%s\n",
               time,
               "criacao",
               process->id,
               process->memory_base,
               process->memory_mb,
               priority_name(process->priority));

        if (enqueue_ready(system, process) != 0) {
            fprintf(stderr, "Erro ao enfileirar P%d.\n", process->id);
            complete_process(system, time, process);
        }
    }
}

static void start_io(SingleThreadSystem *system, int time)
{
    for (int disk = 0; disk < SYSTEM_DISK_COUNT; disk++) {
        if (system->disks[disk].process != NULL) {
            continue;
        }

        Process *process = queue_pop(&system->io_waiting);
        if (process == NULL) {
            return;
        }

        system->disks[disk].process = process;
        printf("  t=%03d | %-9s | P%-3d | disco%d iniciou I/O\n",
               time,
               "io",
               process->id,
               disk);
    }
}

static void dispatch_cpus(SingleThreadSystem *system, int time)
{
    for (int cpu = 0; cpu < SYSTEM_CPU_COUNT; cpu++) {
        if (system->cpus[cpu].process != NULL) {
            continue;
        }

        Process *process = pop_next_ready(system);
        if (process == NULL) {
            return;
        }

        if (process->priority == PROCESS_USER && process->quantum_remaining <= 0) {
            process->quantum_remaining = USER_QUANTUM;
        }
        set_state(time, process, PROCESS_RUNNING);
        system->cpus[cpu].process = process;
        if (process->priority == PROCESS_REAL_TIME) {
            printf("  t=%03d | %-9s | P%-3d | cpu%d fila=tempo-real quantum=-\n",
                   time,
                   "despacho",
                   process->id,
                   cpu);
        } else {
            printf("  t=%03d | %-9s | P%-3d | cpu%d fila=%d quantum=%d\n",
                   time,
                   "despacho",
                   process->id,
                   cpu,
                   process->feedback_level,
                   process->quantum_remaining);
        }
    }
}

static void finish_cpu_phase(SingleThreadSystem *system, int time, int cpu, Process *process)
{
    system->cpus[cpu].process = NULL;

    if (process->phase == PHASE_CPU1 && process->io_time > 0) {
        process->phase = PHASE_IO;
        process->remaining_time = process->io_time;
        set_state(time, process, PROCESS_BLOCKED_IO);
        if (queue_push(&system->io_waiting, process) != 0) {
            fprintf(stderr, "Erro ao enfileirar I/O de P%d.\n", process->id);
            complete_process(system, time, process);
        }
        return;
    }

    if (process->phase == PHASE_CPU1 && process->cpu2_time > 0) {
        process->phase = PHASE_CPU2;
        process->remaining_time = process->cpu2_time;
        process->quantum_remaining = USER_QUANTUM;
        set_state(time, process, PROCESS_READY);
        enqueue_ready(system, process);
        return;
    }

    complete_process(system, time, process);
}

static void tick_cpus(SingleThreadSystem *system, int time)
{
    for (int cpu = 0; cpu < SYSTEM_CPU_COUNT; cpu++) {
        Process *process = system->cpus[cpu].process;
        if (process == NULL) {
            continue;
        }

        process->remaining_time--;
        if (process->priority == PROCESS_USER) {
            process->quantum_remaining--;
        }

        if (process->remaining_time <= 0) {
            finish_cpu_phase(system, time + 1, cpu, process);
            continue;
        }

        if (process->priority == PROCESS_USER && process->quantum_remaining <= 0) {
            system->cpus[cpu].process = NULL;
            if (process->feedback_level < 2) {
                process->feedback_level++;
            }
            process->quantum_remaining = USER_QUANTUM;
            set_state(time + 1, process, PROCESS_READY);
            enqueue_ready(system, process);
        }
    }
}

static void tick_disks(SingleThreadSystem *system, int time)
{
    for (int disk = 0; disk < SYSTEM_DISK_COUNT; disk++) {
        Process *process = system->disks[disk].process;
        if (process == NULL) {
            continue;
        }

        process->remaining_time--;
        if (process->remaining_time > 0) {
            continue;
        }

        system->disks[disk].process = NULL;
        if (process->cpu2_time > 0) {
            process->phase = PHASE_CPU2;
            process->remaining_time = process->cpu2_time;
            process->quantum_remaining = USER_QUANTUM;
            set_state(time + 1, process, PROCESS_READY);
            enqueue_ready(system, process);
        } else {
            complete_process(system, time + 1, process);
        }
    }
}

static void print_resources(const SingleThreadSystem *system, int time)
{
    printf("\n  Recursos em t=%03d\n", time);
    printf("  CPUs   ");
    for (int cpu = 0; cpu < SYSTEM_CPU_COUNT; cpu++) {
        if (system->cpus[cpu].process == NULL) {
            printf("| cpu%-2d: %-8s ", cpu, "livre");
        } else {
            char label[16];
            snprintf(label, sizeof(label), "P%d", system->cpus[cpu].process->id);
            printf("| cpu%-2d: %-8s ", cpu, label);
        }
    }
    printf("|\n");

    printf("  Discos ");
    for (int disk = 0; disk < SYSTEM_DISK_COUNT; disk++) {
        if (system->disks[disk].process == NULL) {
            printf("| d%-4d: %-8s ", disk, "livre");
        } else {
            char label[16];
            snprintf(label, sizeof(label), "P%d", system->disks[disk].process->id);
            printf("| d%-4d: %-8s ", disk, label);
        }
    }
    printf("|\n");
}

static void print_queues(const SingleThreadSystem *system)
{
    printf("  Filas  | tempo-real=%zu | usuario0=%zu | usuario1=%zu | usuario2=%zu | io=%zu |\n",
           system->real_time_ready.count,
           system->user_ready[0].count,
           system->user_ready[1].count,
           system->user_ready[2].count,
           system->io_waiting.count);
}

static int run_single(ProcessList *processes)
{
    SingleThreadSystem system;
    if (init_system(&system) != 0) {
        fprintf(stderr, "Erro ao inicializar memoria do sistema.\n");
        return 1;
    }

    int time = 0;
    while (system.finished_count < (int)processes->count) {
        printf("\n");
        print_rule();
        printf("Ciclo %d: t=%03d -> t=%03d\n", time, time, time + 1);
        print_rule();
        admit_processes(&system, processes, time);
        start_io(&system, time);
        dispatch_cpus(&system, time);
        print_resources(&system, time);
        print_queues(&system);
        memory_print(&system.memory);
        tick_cpus(&system, time);
        tick_disks(&system, time);
        time++;
    }

    printf("\n");
    print_rule();
    printf("Simulacao finalizada em t=%d. Processos finalizados: %d.\n",
           time,
           system.finished_count);
    print_rule();
    destroy_system(&system);
    return 0;
}

int simulator_run(SimMode mode, const char *input_path)
{
    if (mode == SIM_MODE_MULTI) {
        fprintf(stderr, "Modo multi ainda nao implementado.\n");
        return 1;
    }

    ProcessList processes;
    process_list_init(&processes);

    if (process_list_load(&processes, input_path) != 0) {
        process_list_destroy(&processes);
        return 1;
    }

    print_rule();
    printf("Simulador de Escalonamento - modo single-thread\n");
    printf("Processos carregados: %zu | CPUs: %d | Discos: %d | Memoria: %d MiB\n",
           processes.count,
           SYSTEM_CPU_COUNT,
           SYSTEM_DISK_COUNT,
           SYSTEM_MEMORY_MB);
    print_rule();

    int status = run_single(&processes);
    process_list_destroy(&processes);
    return status;
}
