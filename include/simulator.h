#ifndef SIMULATOR_H
#define SIMULATOR_H

#define SYSTEM_CPU_COUNT 4
#define SYSTEM_DISK_COUNT 4
#define SYSTEM_MEMORY_MB (32 * 1024)
#define USER_QUANTUM 2

int simulator_run(const char *input_path, const char *html_path);

#endif
