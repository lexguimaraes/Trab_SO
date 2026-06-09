#include "simulator.h"

#include "process.h"

#include <stdio.h>

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

    printf("Simulador single-thread iniciado com %zu processo(s).\n", processes.count);

    process_list_destroy(&processes);
    return 0;
}
