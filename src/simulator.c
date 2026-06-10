#include "simulator.h"

#include "memory.h"
#include "process.h"
#include "process_queue.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    Process *process;
} CpuSlot;

typedef struct {
    Process *process;
} DiskSlot;

typedef struct {
    FILE *file;
    const char *path;
} HtmlReport;

typedef struct {
    ProcessQueue real_time_ready;
    ProcessQueue user_ready[3];
    ProcessQueue io_waiting;
    CpuSlot cpus[SYSTEM_CPU_COUNT];
    DiskSlot disks[SYSTEM_DISK_COUNT];
    MemoryManager memory;
    HtmlReport *report;
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

static void html_write_header(HtmlReport *report, const ProcessList *processes)
{
    if (report == NULL || report->file == NULL) {
        return;
    }

    fprintf(report->file,
            "<!doctype html>\n"
            "<html lang=\"pt-BR\">\n"
            "<head>\n"
            "  <meta charset=\"utf-8\">\n"
            "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
            "  <title>Resultado da Simulacao</title>\n"
            "  <style>\n"
            "    :root { color-scheme: light; --bg:#f6f7f9; --panel:#fff; --line:#d9dee7; --text:#172033; --muted:#667085; --accent:#1f6feb; --ok:#1a7f37; --warn:#b45309; --rt:#7c3aed; --user:#0f766e; }\n"
            "    * { box-sizing: border-box; }\n"
            "    body { margin: 0; font: 14px/1.45 system-ui, -apple-system, Segoe UI, sans-serif; background: var(--bg); color: var(--text); }\n"
            "    header { position: sticky; top: 0; z-index: 2; background: #101828; color: white; padding: 18px 28px; box-shadow: 0 2px 12px rgba(15,23,42,.2); }\n"
            "    h1 { margin: 0 0 8px; font-size: 22px; letter-spacing: 0; }\n"
            "    h2 { margin: 0 0 14px; font-size: 18px; }\n"
            "    h3 { margin: 18px 0 8px; font-size: 14px; color: var(--muted); text-transform: uppercase; }\n"
            "    main { padding: 24px 28px 40px; max-width: 1280px; margin: 0 auto; }\n"
            "    .summary { display: flex; flex-wrap: wrap; gap: 10px; }\n"
            "    .pill { display: inline-flex; gap: 6px; align-items: center; padding: 5px 10px; border: 1px solid rgba(255,255,255,.25); border-radius: 999px; color: #e5e7eb; }\n"
            "    .cycle { background: var(--panel); border: 1px solid var(--line); border-radius: 8px; margin: 0 0 18px; overflow: hidden; }\n"
            "    .cycle-title { display:flex; justify-content:space-between; gap:12px; padding: 12px 14px; border-bottom: 1px solid var(--line); background:#fbfcfe; }\n"
            "    .cycle-body { padding: 14px; display: grid; gap: 14px; }\n"
            "    .events { width: 100%%; border-collapse: collapse; }\n"
            "    .events th, .events td { padding: 7px 8px; border-bottom: 1px solid #eef1f5; text-align: left; vertical-align: top; }\n"
            "    .events th { color: var(--muted); font-size: 12px; font-weight: 700; background: #fbfcfe; }\n"
            "    .tag { display:inline-block; min-width:72px; text-align:center; padding:2px 7px; border-radius:999px; font-size:12px; font-weight:700; background:#eef4ff; color:#194185; }\n"
            "    .grid { display:grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 10px; }\n"
            "    .resource { border:1px solid var(--line); border-radius:8px; padding:10px; background:#fff; }\n"
            "    .resource strong { display:block; margin-bottom:8px; }\n"
            "    .slots { display:grid; grid-template-columns: repeat(4, minmax(0,1fr)); gap:8px; }\n"
            "    .slot { min-height:38px; display:flex; align-items:center; justify-content:center; border:1px solid var(--line); border-radius:6px; background:#f8fafc; font-weight:700; }\n"
            "    .busy { background:#e8f1ff; border-color:#b7d4ff; color:#0b4a8b; }\n"
            "    .queues { display:flex; flex-wrap:wrap; gap:8px; }\n"
            "    .queue { padding:6px 9px; border-radius:6px; background:#f8fafc; border:1px solid var(--line); }\n"
            "    .memory { display:flex; min-height:42px; border:1px solid var(--line); border-radius:8px; overflow:hidden; background:#fff; }\n"
            "    .block { min-width:70px; padding:7px 8px; border-right:1px solid rgba(255,255,255,.7); overflow:hidden; white-space:nowrap; text-overflow:ellipsis; font-size:12px; }\n"
            "    .free { background:#edf2f7; color:#475467; }\n"
            "    .used { background:#dbeafe; color:#0b4a8b; }\n"
            "    .final { background:var(--panel); border:1px solid var(--line); border-radius:8px; padding:16px; }\n"
            "    table.processes { width:100%%; border-collapse: collapse; }\n"
            "    table.processes th, table.processes td { padding:8px; border-bottom:1px solid #eef1f5; text-align:left; }\n"
            "    .priority-rt { color: var(--rt); font-weight:700; }\n"
            "    .priority-user { color: var(--user); font-weight:700; }\n"
            "  </style>\n"
            "</head>\n"
            "<body>\n"
            "<header>\n"
            "  <h1>Resultado da Simulacao</h1>\n"
            "  <div class=\"summary\">\n"
            "    <span class=\"pill\">Processos: %zu</span>\n"
            "    <span class=\"pill\">CPUs: %d</span>\n"
            "    <span class=\"pill\">Discos: %d</span>\n"
            "    <span class=\"pill\">Memoria: %d MiB</span>\n"
            "    <span class=\"pill\">Quantum usuario: %d</span>\n"
            "  </div>\n"
            "</header>\n"
            "<main>\n",
            processes->count,
            SYSTEM_CPU_COUNT,
            SYSTEM_DISK_COUNT,
            SYSTEM_MEMORY_MB,
            USER_QUANTUM);
}

static void html_cycle_begin(HtmlReport *report, int time)
{
    if (report == NULL || report->file == NULL) {
        return;
    }

    fprintf(report->file,
            "<section class=\"cycle\">\n"
            "  <div class=\"cycle-title\"><h2>Ciclo %d</h2><span>t=%03d &rarr; t=%03d</span></div>\n"
            "  <div class=\"cycle-body\">\n"
            "    <table class=\"events\">\n"
            "      <thead><tr><th>Tempo</th><th>Tipo</th><th>Processo</th><th>Detalhe</th></tr></thead>\n"
            "      <tbody>\n",
            time,
            time,
            time + 1);
}

static void html_event(HtmlReport *report, int time, const char *type, int pid, const char *message)
{
    if (report == NULL || report->file == NULL) {
        return;
    }

    fprintf(report->file,
            "        <tr><td>t=%03d</td><td><span class=\"tag\">%s</span></td><td>P%d</td><td>%s</td></tr>\n",
            time,
            type,
            pid,
            message);
}

static void html_transition(HtmlReport *report, int time, const Process *process, ProcessState previous)
{
    char message[160];
    snprintf(message,
             sizeof(message),
             "%s -> %s | fase=%s | restante=%d",
             process_state_name(previous),
             process_state_name(process->state),
             process_phase_name(process->phase),
             process->remaining_time);
    html_event(report, time, "estado", process->id, message);
}

static void html_snapshot(HtmlReport *report, const SingleThreadSystem *system, int time)
{
    if (report == NULL || report->file == NULL) {
        return;
    }

    fprintf(report->file,
            "      </tbody>\n"
            "    </table>\n"
            "    <div class=\"grid\">\n"
            "      <div class=\"resource\"><strong>CPUs em t=%03d</strong><div class=\"slots\">",
            time);

    for (int cpu = 0; cpu < SYSTEM_CPU_COUNT; cpu++) {
        const Process *process = system->cpus[cpu].process;
        if (process == NULL) {
            fprintf(report->file, "<div class=\"slot\">cpu%d<br>livre</div>", cpu);
        } else {
            fprintf(report->file, "<div class=\"slot busy\">cpu%d<br>P%d</div>", cpu, process->id);
        }
    }

    fprintf(report->file,
            "</div></div>\n"
            "      <div class=\"resource\"><strong>Discos em t=%03d</strong><div class=\"slots\">",
            time);

    for (int disk = 0; disk < SYSTEM_DISK_COUNT; disk++) {
        const Process *process = system->disks[disk].process;
        if (process == NULL) {
            fprintf(report->file, "<div class=\"slot\">d%d<br>livre</div>", disk);
        } else {
            fprintf(report->file, "<div class=\"slot busy\">d%d<br>P%d</div>", disk, process->id);
        }
    }

    fprintf(report->file,
            "</div></div>\n"
            "    </div>\n"
            "    <div class=\"queues\">\n"
            "      <span class=\"queue\">tempo-real: %zu</span>\n"
            "      <span class=\"queue\">usuario0: %zu</span>\n"
            "      <span class=\"queue\">usuario1: %zu</span>\n"
            "      <span class=\"queue\">usuario2: %zu</span>\n"
            "      <span class=\"queue\">io: %zu</span>\n"
            "    </div>\n"
            "    <div class=\"memory\">\n",
            system->real_time_ready.count,
            system->user_ready[0].count,
            system->user_ready[1].count,
            system->user_ready[2].count,
            system->io_waiting.count);

    for (size_t i = 0; i < system->memory.count; i++) {
        const MemoryBlock *block = &system->memory.blocks[i];
        int flex = block->size > 0 ? block->size : 1;
        if (block->free) {
            fprintf(report->file,
                    "      <div class=\"block free\" style=\"flex:%d\">base=%d<br>%d MiB livre</div>\n",
                    flex,
                    block->base,
                    block->size);
        } else {
            fprintf(report->file,
                    "      <div class=\"block used\" style=\"flex:%d\">P%d<br>base=%d<br>%d MiB</div>\n",
                    flex,
                    block->pid,
                    block->base,
                    block->size);
        }
    }

    fprintf(report->file, "    </div>\n");
}

static void html_cycle_end(HtmlReport *report)
{
    if (report == NULL || report->file == NULL) {
        return;
    }

    fprintf(report->file, "  </div>\n</section>\n");
}

static void html_write_footer(HtmlReport *report, const ProcessList *processes, int final_time)
{
    if (report == NULL || report->file == NULL) {
        return;
    }

    fprintf(report->file,
            "<section class=\"final\">\n"
            "  <h2>Resumo Final</h2>\n"
            "  <p>Simulacao finalizada em <strong>t=%d</strong>.</p>\n"
            "  <table class=\"processes\">\n"
            "    <thead><tr><th>PID</th><th>Prioridade</th><th>CPU1</th><th>I/O</th><th>CPU2</th><th>Memoria</th><th>Estado final</th></tr></thead>\n"
            "    <tbody>\n",
            final_time);

    for (size_t i = 0; i < processes->count; i++) {
        const Process *process = &processes->items[i];
        const char *priority_class =
            process->priority == PROCESS_REAL_TIME ? "priority-rt" : "priority-user";
        fprintf(report->file,
                "      <tr><td>P%d</td><td class=\"%s\">%s</td><td>%d</td><td>%d</td><td>%d</td><td>%d MiB</td><td>%s</td></tr>\n",
                process->id,
                priority_class,
                priority_name(process->priority),
                process->cpu1_time,
                process->io_time,
                process->cpu2_time,
                process->memory_mb,
                process_state_name(process->state));
    }

    fprintf(report->file,
            "    </tbody>\n"
            "  </table>\n"
            "</section>\n"
            "</main>\n"
            "</body>\n"
            "</html>\n");
}

static void log_transition(SingleThreadSystem *system,
                           int time,
                           const Process *process,
                           ProcessState previous)
{
    printf("  t=%03d | %-9s | P%-3d | %-10s -> %-10s | fase=%-4s restante=%d\n",
           time,
           "estado",
           process->id,
           process_state_name(previous),
           process_state_name(process->state),
           process_phase_name(process->phase),
           process->remaining_time);
    html_transition(system->report, time, process, previous);
}

static void set_state(SingleThreadSystem *system, int time, Process *process, ProcessState state)
{
    ProcessState previous = process->state;
    process->state = state;
    if (previous != state) {
        log_transition(system, time, process, previous);
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
    set_state(system, time, process, PROCESS_FINISHED);
    memory_release(&system->memory, process->id);
    system->finished_count++;
    print_event(time, "memoria", process->id, "finalizado; memoria liberada");
    html_event(system->report, time, "memoria", process->id, "finalizado; memoria liberada");
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
    system->report = NULL;
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
        set_state(system, time, process, PROCESS_READY);
        printf("  t=%03d | %-9s | P%-3d | base=%-5d tamanho=%-5d MiB prioridade=%s\n",
               time,
               "criacao",
               process->id,
               process->memory_base,
               process->memory_mb,
               priority_name(process->priority));
        char message[160];
        snprintf(message,
                 sizeof(message),
                 "base=%d | tamanho=%d MiB | prioridade=%s",
                 process->memory_base,
                 process->memory_mb,
                 priority_name(process->priority));
        html_event(system->report, time, "criacao", process->id, message);

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
        char message[64];
        snprintf(message, sizeof(message), "disco%d iniciou I/O", disk);
        html_event(system->report, time, "io", process->id, message);
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
        set_state(system, time, process, PROCESS_RUNNING);
        system->cpus[cpu].process = process;
        if (process->priority == PROCESS_REAL_TIME) {
            printf("  t=%03d | %-9s | P%-3d | cpu%d fila=tempo-real quantum=-\n",
                   time,
                   "despacho",
                   process->id,
                   cpu);
            char message[80];
            snprintf(message, sizeof(message), "cpu%d | fila=tempo-real | quantum=-", cpu);
            html_event(system->report, time, "despacho", process->id, message);
        } else {
            printf("  t=%03d | %-9s | P%-3d | cpu%d fila=%d quantum=%d\n",
                   time,
                   "despacho",
                   process->id,
                   cpu,
                   process->feedback_level,
                   process->quantum_remaining);
            char message[80];
            snprintf(message,
                     sizeof(message),
                     "cpu%d | fila=%d | quantum=%d",
                     cpu,
                     process->feedback_level,
                     process->quantum_remaining);
            html_event(system->report, time, "despacho", process->id, message);
        }
    }
}

static void finish_cpu_phase(SingleThreadSystem *system, int time, int cpu, Process *process)
{
    system->cpus[cpu].process = NULL;

    if (process->phase == PHASE_CPU1 && process->io_time > 0) {
        process->phase = PHASE_IO;
        process->remaining_time = process->io_time;
        set_state(system, time, process, PROCESS_BLOCKED_IO);
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
        set_state(system, time, process, PROCESS_READY);
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
            set_state(system, time + 1, process, PROCESS_READY);
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
            set_state(system, time + 1, process, PROCESS_READY);
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

static int run_single(ProcessList *processes, const char *html_path)
{
    SingleThreadSystem system;
    if (init_system(&system) != 0) {
        fprintf(stderr, "Erro ao inicializar memoria do sistema.\n");
        return 1;
    }

    HtmlReport report = {
        .file = NULL,
        .path = html_path,
    };
    if (html_path != NULL) {
        report.file = fopen(html_path, "w");
        if (report.file == NULL) {
            fprintf(stderr, "Erro ao criar relatorio HTML '%s': %s\n", html_path, strerror(errno));
            destroy_system(&system);
            return 1;
        }
        system.report = &report;
        html_write_header(&report, processes);
    }

    int time = 0;
    while (system.finished_count < (int)processes->count) {
        printf("\n");
        print_rule();
        printf("Ciclo %d: t=%03d -> t=%03d\n", time, time, time + 1);
        print_rule();
        html_cycle_begin(system.report, time);
        admit_processes(&system, processes, time);
        start_io(&system, time);
        dispatch_cpus(&system, time);
        print_resources(&system, time);
        print_queues(&system);
        memory_print(&system.memory);
        tick_cpus(&system, time);
        tick_disks(&system, time);
        html_snapshot(system.report, &system, time + 1);
        html_cycle_end(system.report);
        time++;
    }

    printf("\n");
    print_rule();
    printf("Simulacao finalizada em t=%d. Processos finalizados: %d.\n",
           time,
           system.finished_count);
    print_rule();
    if (system.report != NULL) {
        html_write_footer(system.report, processes, time);
        fclose(system.report->file);
        printf("Relatorio HTML gerado em: %s\n", system.report->path);
    }
    destroy_system(&system);
    return 0;
}

int simulator_run(SimMode mode, const char *input_path, const char *html_path)
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

    int status = run_single(&processes, html_path);
    process_list_destroy(&processes);
    return status;
}
