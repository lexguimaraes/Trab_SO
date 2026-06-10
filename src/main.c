#include "simulator.h"

#include <stdio.h>
#include <string.h>

static void print_usage(const char *program)
{
    fprintf(stderr, "Uso: %s --modo <single|multi> <arquivo> [--html resultado.html]\n", program);
}

int main(int argc, char **argv)
{
    if ((argc != 4 && argc != 6) || strcmp(argv[1], "--modo") != 0) {
        print_usage(argv[0]);
        return 1;
    }

    SimMode mode;
    if (strcmp(argv[2], "single") == 0) {
        mode = SIM_MODE_SINGLE;
    } else if (strcmp(argv[2], "multi") == 0) {
        mode = SIM_MODE_MULTI;
    } else {
        print_usage(argv[0]);
        return 1;
    }

    const char *html_path = NULL;
    if (argc == 6) {
        if (strcmp(argv[4], "--html") != 0) {
            print_usage(argv[0]);
            return 1;
        }
        html_path = argv[5];
    }

    return simulator_run(mode, argv[3], html_path);
}
