#ifndef SIMULATOR_H
#define SIMULATOR_H

#define SYSTEM_CPU_COUNT 4
#define SYSTEM_DISK_COUNT 4
#define SYSTEM_MEMORY_MB (32 * 1024)
#define USER_QUANTUM 2

typedef enum {
    SIM_MODE_SINGLE,
    SIM_MODE_MULTI
} SimMode;

int simulator_run(SimMode mode, const char *input_path, const char *html_path);

#endif
