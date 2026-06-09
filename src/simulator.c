#include "simulator.h"

#include <stdio.h>

int simulator_run(SimMode mode, const char *input_path)
{
    (void)input_path;

    if (mode == SIM_MODE_MULTI) {
        fprintf(stderr, "Modo multi ainda nao implementado.\n");
        return 1;
    }

    printf("Simulador single-thread iniciado.\n");
    return 0;
}
