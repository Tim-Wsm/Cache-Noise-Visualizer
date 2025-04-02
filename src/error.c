/**
 * @file error.c
 * @date 9 Nov 2017
 *
 * @brief Contains all functions which are necessary for the error handling.
 */
#include "error.h"

const error_unit_t error_map_table[] = {
    {ERROR_IO, "during IO interaction (ERROR_IO)"},
    {ERROR_ALLOCATION, "allocating memory (ERROR_ALLOCATION)"},
    {ERROR_FD_CYCLE,
     "opening a file descriptor to cycle event (ERROR_FD_CYCLE)"},
    {ERROR_FD_CYCLE_CLOSE,
     "closing a file descriptor to cycle event (ERROR_FD_CYCLE)"},
    {ERROR_FD_CACHE,
     "opening a file descriptor to cache event(ERROR_FD_CACHE)"},
    {ERROR_SET_AFFINITY, "setting the CPU affinity(ERROR_SET_AFFINITY)"},
    {ERROR_SYSCONF, "getting value from sysconf"},
    {ERROR_PAGE_ENTRY, "retrieving a page entry"},
    {ERROR_NOT_ALIGNED, "the buffer is not aligned to the cache"},
    {ERROR_MMAP, "using while using mmap (ERROR_MMAP)"},
    {ERROR_CACHE_NOT_EXISTS, "cache does not exist (ERROR_CACHE_NOT_EXISTS)"},
    {ERROR_NO_HUGEPAGES, "no hugepages are available (ERROR_NO_HUGEPAGES)"},
    {ERROR_MUNMAP, "failed to remove memory mappings (ERROR_MUNMAP)"},
    {ERROR_SPAWN_SPAWN, "failed to spawn the process (ERROR_SPAWN_SPAWN)"},
    {ERROR_SPAWN_ATTR_INIT,
     "failed to initialize `attr` attribute (ERROR_SPAWN_ATTR_INIT)"},
    {ERROR_SPAWN_FILE_INIT,
     "failed to initialize `file` attribute (ERROR_SPAWN_FILLE_INIT)"},
    {ERROR_SPAWN_PATH_NULL,
     "the given path to the program is null. (ERROR_SPAWN_PATH_NULL)"},
    {ERROR_SET_SCHEDULER_POLICY,
     "setting the scheduler policy (ERROR_SET_SCHEDULER_POLICY)"},
    {ERROR_GET_CPU_CORE,
     "retrieving the CPU core on which the program currently runs on"},
    {ERROR_IO_PROC_SELF_PAGEMAP, "during IO interaction with "
                                 "/proc/self/pagemap "
                                 "(ERROR_IO_PROC_SELF_PAGEMAP)"},
    {ERROR_IO_PROC_MEMINFO,
     "during IO interaction with /proc/meminfo (ERROR_IO_PROC_MEMINFO)"},
    {ERROR_IO_HDF,
     "during IO interaction with the HDF output file (ERROR_IO_HDF)"},
    {ERROR_IO_SYS_CPU,
     "during IO interaction in the /sys/cpu/ filestructure (ERROR_IO_SYS_CPU)"},
    {ERROR_IO_TOTAL_SIZE, "during IO interaction with "
                          "/sys/devices/system/cpu/cpu%d/cache/index%d/size "
                          "(ERROR_IO_TOTAL_SIZE)"},
    {ERROR_IO_LINE_SIZE, "during IO interaction with "
                         "/sys/devices/system/cpu/cpu%d/cache/index%d/"
                         "coherency_line_size (ERROR_IO_LINE_SIZE)"},
    {ERROR_IO_SET_NUMBER, "during IO interaction with "
                          "/sys/devices/system/cpu/cpu%d/cache/index%d/"
                          "number_of_sets (ERROR_IO_SET_NUMBER)"},
    {ERROR_IO_LEVELS, "during IO interaction with "
                      "/sys/devices/system/cpu/cpu%d/cache/index%d/level "
                      "(ERROR_IO_LEVELS)"},
    {ERROR_IO_ASSOCIATIVITY, "during IO interaction with "
                             "/sys/devices/system/cpu/cpu%d/cache/index%d/"
                             "ways_of_assoziativity (ERROR_IO_ASSOCIATIVITY)"},
    {ERROR_IO_HUGEPAGE_NUMBER, "during IO interaction with "
                               "/proc/sys/vm/nr_hugepages "
                               "(ERROR_IO_HUGEPAGE_NUMBER)"},
    {ERROR_IO_RDPMC, "during IO interaction with "
                     "/sys/bus/event_source/devices/cpu/rdpmc "
                     "(ERROR_IO_RDPMC)"},
    {ERROR_RDPMC,
     "the rdpmc instruction is not available in userspace (ERROR_RDPMC)"}};

const char *default_error_message = "unknown error";

const char *decode_error(error_t error) {
    size_t len = sizeof(error_map_table) / sizeof(error_map_table[0]);

    for (int i = 0; i < len; i++) {
        if (error_map_table[i].number == error) {
            return error_map_table[i].message;
        }
    }

    return default_error_message;
}
