/**
 * @file sys_info.c
 * @date 9 Nov 2017
 *
 * @brief Contains all functions which are necessary to retrieve system
 * information.
 *
 * This module provides functions to get important cache information
 * and translate functions for virtual addresses.
 */
#define _GNU_SOURCE /* needed for CPU_ZERO, CPU_SET and sched_setaffinity*/

#include "sys_info.h"

#include <ctype.h>
#include <dirent.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * The file /dev/self/pagemap contains all available virtual memory
 * addresses for the current process. Every entry is 64bit large.
 */
#define PAGE_ENTRY_SIZE 8

/**
 * @brief Checks if a string starts with a given prefix.
 * @param pre Prefix
 * @param str The string which can contain the prefix.
 *
 * Checks if @p pre is prefix of string @p str.
 *
 * @return Zero if the string does not start with the prefix. Otherwise returns
 * non Zero.
 */
int str_starts_with(const char *pre, const char *str);

/**
 * @brief Reads a unsigend int from a file.
 * @param fname File path
 * @param ret Writes the value from the file into this.
 *
 * Opens a file and tries to scan the file for an unsigned integer. And
 * writes the value into the given pointer @p ret.
 *
 * @retval ERROR_IO
 * @retval ERROR_FMT
 * @retval ERROR_NONE
 *
 * @return An error code which is defined in error.h
 */
error_t read_file_uint(const char *fname, unsigned int *ret);

#define READ_PROP_PUT(path, filename, write_back, suffix)                      \
    {                                                                          \
        size_t new_path_size =                                                 \
            sizeof(char) * (strlen(path) + strlen(filename)) + 1;              \
        char new_path[(strlen(path) + strlen(filename)) + 1];                  \
        if (new_path == NULL) {                                                \
            return ERROR_ALLOCATION;                                           \
        }                                                                      \
        strncpy(new_path, path, new_path_size - 1);                            \
        strncat(new_path, filename, new_path_size - strlen(new_path) - 1);     \
        FILE *file;                                                            \
        file = fopen(new_path, "r");                                           \
        if (!file) {                                                           \
            return ERROR_IO_##suffix;                                          \
        }                                                                      \
        int error = fscanf(file, "%u", write_back);                            \
        fclose(file);                                                          \
        if (error == EOF) {                                                    \
            return ERROR_FMT;                                                  \
        }                                                                      \
    }

int str_starts_with(const char *pre, const char *str) {
    size_t len_pre = strlen(pre);
    size_t len_str = strlen(str);
    return len_str < len_pre ? 0 : strncmp(pre, str, len_pre) == 0;
}

/**
 * @brief Filles a cache_info with information about the system cache.
 *
 * @param info pointer to cache_info_t
 * @param cache the internal id of the cache
 * @param cpu the internal id of the cpu
 *
 * On Linux the information are retrieved from the sysfs file system.
 *
 * @retval ERROR_IO
 * @retval ERROR_FMT
 * @retval ERROR_ALLOCATION
 * @retval ERROR_NONE
 *
 * @return An error code which is defined in error.h
 */
error_t cache_info_fill(cache_info_t *info, uint32_t cache, uint32_t cpu) {
    char cpu_path[60];
    sprintf(cpu_path, "/sys/devices/system/cpu/cpu%d/cache/index%d/", cpu,
            cache);

    READ_PROP_PUT(cpu_path, "coherency_line_size", &info->line_size, LINE_SIZE);
    READ_PROP_PUT(cpu_path, "size", &info->total_size, TOTAL_SIZE);
    info->total_size *= 1024;
    READ_PROP_PUT(cpu_path, "number_of_sets", &info->set_count, SET_NUMBER);
    READ_PROP_PUT(cpu_path, "level", &info->level, LEVELS);
    READ_PROP_PUT(cpu_path, "ways_of_associativity",
                  &info->ways_of_associativity, ASSOCIATIVITY);

    info->cpu_id = cpu;
    info->cache_id = cache;

    char *type = "type";
    size_t new_path_size = sizeof(char) * (strlen(cpu_path) + strlen(type) + 1);
    char *new_path = malloc(new_path_size);
    strncpy(new_path, cpu_path, new_path_size - 1);
    strncat(new_path, type, new_path_size - strlen(new_path) - 1);

    FILE *file;
    file = fopen(new_path, "r");

    char c = fgetc(file);
    free(new_path);
    cache_type_t ctype;

    if (c == 'D') {
        ctype = DATA;
    } else if (c == 'I') {
        ctype = INSTRUCTION;
    } else if (c == 'U') {
        if (fgetc(file) == 'n') {
            if (fgetc(file) == 'i') {
                ctype = UNIFIED;
            } else {
                ctype = UNKOWN;
            }
        } else {
            ctype = UNKOWN;
        }
    } else {
        ctype = UNKOWN;
    }

    info->type = ctype;

    fclose(file);

    return ERROR_NONE;
}

/**
 * @brief Counts the caches on a given CPU.
 * @param cpu The internal CPU id.
 * @param count Writes back the count value.
 *
 * Counts the amount of caches on a CPU. It does not matter which
 * level the caches have. It uses the Linux sysfs where each cache
 * has a different index. The range of these indexes is from zero to
 * n.
 *
 * @retval ERROR_IO
 * @retval ERROR_NONE
 *
 * @return An error code which is defined in error.h
 */
error_t cache_count(uint32_t cpu, uint32_t *count) {
    char cpu_path[256];
    sprintf(cpu_path, "/sys/devices/system/cpu/cpu%d/cache/", cpu);

    *count = 0;

    DIR *dp;
    struct dirent *ep;
    dp = opendir(cpu_path);

    if (dp != NULL) {
        while ((ep = readdir(dp))) {
            if (str_starts_with("index", ep->d_name)) {
                *count += 1;
            }
        }
        closedir(dp);
    } else {
        return ERROR_IO_SYS_CPU;
    }

    return ERROR_NONE;
}

void cache_info_print(const cache_info_t *info) {
    if (info != NULL) {
        printf("L%d CACHE (has index %d) on CPU %d: \n", info->level,
               info->cache_id, info->cpu_id);
        printf("  LINE SIZE: %d\n", info->line_size);
        printf("  TOTAL SIZE: %d\n", info->total_size);
        printf("  SET COUNT: %d\n", info->set_count);
        printf("  ASSOCIATIVITY COUNT: %d\n", info->ways_of_associativity);
        printf("  TYPE: %s\n",
               info->type == DATA
                   ? "DATA"
                   : info->type == INSTRUCTION
                         ? "INSTRUCTION"
                         : info->type == UNIFIED ? "UNIFIED" : "UNKNOWN");
    } else {
        fprintf(stderr, "Could not print cache info -> Pointer == NULL");
    }
}

error_t cache_info_new(cache_info_t *info, uint8_t cpu, uint8_t level) {
    uint32_t count = 0;
    FORWARD_ON_FAIL(cache_count(cpu, &count));

    for (int i = 0; i < count; i++) {
        FORWARD_ON_FAIL(cache_info_fill(info, i, cpu));
        if (info->type == DATA || info->type == UNIFIED) {
            if (level == info->level) {
                break;
            }
        }
    }

    if (level != info->level) {
        return ERROR_CACHE_NOT_EXISTS;
    }

    return ERROR_NONE;
}

error_t pagemap_entry_from(pagemap_entry_t *entry, const uint64_t offset) {
    FILE *map = fopen("/proc/self/pagemap", "rb");

    if (map == NULL) {
        return ERROR_IO_PROC_SELF_PAGEMAP;
    }

    if (fseek(map, offset, SEEK_SET)) {
        fclose(map);
        return ERROR_IO_PROC_SELF_PAGEMAP;
    }

    uint64_t data = 0;

    if (1 != fread(&data, PAGE_ENTRY_SIZE, 1, map)) {
        fclose(map);
        return ERROR_IO_PROC_SELF_PAGEMAP;
    }

    entry->page_frame_number = data & (((uint64_t)1 << 54) - 1);
    entry->soft_dirty = (data >> 54) & 1;
    entry->file_page = (data >> 61) & 1;
    entry->swapped = (data >> 62) & 1;
    entry->present = (data >> 63) & 1;

    fclose(map);
    return ERROR_NONE;
}

error_t get_physical_address(const void *addr, uintptr_t *target) {
    const uint64_t page_size = sysconf(_SC_PAGESIZE);

    if (page_size < 1) {
        return ERROR_SYSCONF;
    }

    uint64_t page_shift = 0;
    for (uint64_t counter = page_size; counter > 1; counter = counter >> 1) {
        page_shift += 1;
    }

    uint64_t offset = ((uint64_t)addr) / page_size * PAGE_ENTRY_SIZE;

    pagemap_entry_t entry;

    FORWARD_ON_FAIL(pagemap_entry_from(&entry, offset));

    if (!entry.present) {
        return ERROR_PAGE_ENTRY;
    }

    *target = (entry.page_frame_number << page_shift) +
              (((uint64_t)addr) % page_size);

    return ERROR_NONE;
}

error_t get_hugepagenr(uint32_t *nr) {
    READ_PROP_PUT("/proc/sys/vm/", "nr_hugepages", nr, HUGEPAGE_NUMBER);
    return ERROR_NONE;
}

error_t get_hugepagesize(uint64_t *size) {
    FILE *meminfo = fopen("/proc/meminfo", "rb");
    if (meminfo == NULL) {
        return ERROR_IO_PROC_MEMINFO;
    }

    char line[100];
    line[99] = '\0'; // string is 0 terminated

    while (fgets(line, 99, meminfo) != NULL) {
        if (str_starts_with("Hugepagesize:", line)) {
            break;
        }
    }

    if (!str_starts_with("Hugepagesize:", line)) {
        fclose(meminfo);
        return ERROR_IO_PROC_MEMINFO;
    }

    char number[100];
    uint32_t n = 0;
    for (uint32_t i = 0; i < strlen(line) && n < 100; i++) {
        if (isdigit(line[i])) {
            number[n++] = line[i];
        }
    }
    number[n] = '\0';

    *size = atoi(number) * 1024;
    fclose(meminfo);
    return ERROR_NONE;
}

int has_root_access() { return getuid() == 0; }

error_t get_current_cpu_core(uint32_t *core) {
    int32_t res;
    if ((res = sched_getcpu()) < 0) {
        return ERROR_GET_CPU_CORE;
    }
    *core = res;
    return ERROR_NONE;
}

error_t can_use_rdpmc() {
    uint32_t value;
    READ_PROP_PUT("/sys/bus/event_source/devices/cpu/", "rdpmc", &value, RDPMC);

    if (value != 2) {
        return ERROR_RDPMC;
    }

    return ERROR_NONE;
}
