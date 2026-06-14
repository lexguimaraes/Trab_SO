#include "simulator.h"

#include <stdio.h>
#include <string.h>

static void print_usage(const char *program)
{
    fprintf(stderr, "Uso: %s <arquivo> [--html resultado.html]\n", program);
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    const char *input_path = argv[1];

    const char *html_path = NULL;
    if (argc == 4) {
        if (strcmp(argv[2], "--html") != 0) {
            print_usage(argv[0]);
            return 1;
        }
        html_path = argv[3];
    }

    return simulator_run(input_path, html_path);
}
