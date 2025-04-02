/**
 * @file sys_info.h
 * @date 9 Nov 2017
 *
 * @brief Contains functions to retrieve information about the system.
 */
#pragma once

#include "error.h"
#include <stdint.h>
#include <stdio.h>

/**
 * @brief available cache types.
 *
 * cache_type holds the different available cache types.
 * If the cache type can not be determined then the type
 * is UNKNOWN.
 */
typedef enum cache_type {
    DATA,        /**< Cache holds data from the memory. */
    INSTRUCTION, /**< Cache holds CPU instructions. */
    UNIFIED,     /**< Cache holds CPU instructions and data from the memory.*/
    UNKOWN       /**< Could not determine the cache type. */
} cache_type_t;

/**
 * @brief Hold information about a cache.
 *
 * Holds information about a system cache. All size entries
 * are in bytes. On Linux systems this information is retrieved
 * from the sysfs file system.
 */
typedef struct cache_info {
    uint32_t line_size; /**< Line size of the cache. cache_info_t#line_size. */
    uint32_t
        total_size; /**< Total size of the cache. cache_info_t#total_size. */
    uint32_t
        cache_id;    /**< Internal index of the cache. cache_info_t#cache_id */
    uint32_t cpu_id; /**< The internal id of the CPU where the cache is on.
                        cache_info_t#cpu_id. */
    uint32_t set_count; /**< Number of sets. cache_info_t#set_count */
    uint32_t ways_of_associativity; /**< The amount of cache lines in one set.
                                       cache_info_t#ways_of_associativity. */
    uint32_t level; /**< The level in the cache hierarchy. cache_info_t#level */
    cache_type_t type; /**< Specifies the type of the cache . */
} cache_info_t;

/**
 * @brief Allocates an empty cache info.
 *
 * @param cache Writes the cache pointer into.
 *
 * Creates a new instance of cache_info_t.
 *
 * @retval ERROR_IO
 * @retval ERROR_FMT
 * @retval ERROR_ALLOCATION
 * @retval ERROR_NONE
 *
 */
error_t cache_info_new(cache_info_t *info, uint8_t cpu, uint8_t level);

/**
 * @brief Prints cache_info_t to the stdout.
 *
 * @param info Filled cache_info_t
 *
 * Prints all information of the cache into the standard output. If the @p info
 * is NULL then an error message is printed.
 */
void cache_info_print(const cache_info_t *info);

/**
 * @brief An entry in the /proc/[pid]/pagemap file.
 *
 * This is an entry in the /proc/[pid]/pagemap file.
 *
 * Where the following data is extracted from the Linux
 * documentation:
 *
    63     If set, the page is present in RAM.

    62     If set, the page is in swap space

    61 (since Linux 3.5)
            The page is a file-mapped page or a shared anony‐
            mous page.

    60–56 (since Linux 3.11)
            Zero

    55 (since Linux 3.11)
            PTE is soft-dirty (see the kernel source file Doc‐
            umentation/vm/soft-dirty.txt).

    54–0   If the page is present in RAM (bit 63), then these
            bits provide the page frame number, which can be
            used to index /proc/kpageflags and /proc/kpage‐
            count.  If the page is present in swap (bit 62),
            then bits 4–0 give the swap type, and bits 54–5
            encode the swap offset.
 *
 */
typedef struct pagemap_entry {
    uint64_t page_frame_number : 54;
    unsigned int soft_dirty : 1;
    unsigned int file_page : 1;
    unsigned int swapped : 1;
    unsigned int present : 1;
} pagemap_entry_t;

/**
 * @brief Fills the given entry with the information for given the address.
 *
 * @param entry Where the data gets stored.
 * @param offset Offset for the system file.
 *
 * Opens the /proc/self/pagemap to read the information for this process.
 * This means it does not require the root permissions. The offset is used
 * to change the file pointer.
 *
 * | --- 64bit --- | --- 64 bit --- | --- 64 bit --- |  --- 64 bit --- |
 * This is the layout of the pagemap file. The offset determines which
 * entry is read.
 *
 * @retval ERROR_IO_PROC_SELF_PAGEMAP
 * @retval ERROR_NONE
 *
 * @return An error code which is defined in error.h
 */
error_t pagemap_entry_from(pagemap_entry_t *entry, uint64_t offset);

/**
 * @brief Translates the given virtual address into the hardware address.
 *
 * @param addr Any virtual address.
 * @param target Writes the hardware address into the pointer.
 *
 * This takes a valid virtual address and translates it into the hardware
 * address. The result will be written into`target. If an error occurs then
 * error code will be returned (for more information see error). The information
 * are given by sysconf. Since only one value is needed to calculate the other
 * variables which are important for the calculation. Only one sysconf call is
 * done while running this function. For more information checkout the kernel
 * source (macros in the kernel PAGE_SIZE and PAGE_SHIFT).
 *
 * From the Linux kernel:
 *  PAGE_SHIFT is the length in bits of the offset part of the linear address
 *  space. The size of a page is easily calculated as 2^PAGE_SHIFT.
 *
 * @retval ERROR_IO_PROC_SELF_PAGEMAP
 * @retval ERROR_SYSCONF
 * @retval ERROR_NONE
 *
 * @return An error code which is defined in error.h
 */
error_t get_physical_address(const void *addr, uintptr_t *target);

/**
 * @brief Returns the number of possible hugepages in the system
 *
 * @param nr pointer where the result gets placed
 *
 * Checks the system capabilities for hugepages. And saves the number of
 * avialble hugepages into @p nr.
 *
 * @retval ERROR_ALLOCATION
 * @retval ERROR_IO
 * @retval ERROR_FMT
 * @retval ERROR_NONE
 */
error_t get_hugepagenr(uint32_t *nr);

/**
 * @brief Returns the number of size of a hugepage in the system
 *
 * Reads the value Hugepagesize from /proc/meminfo and places it into *size.
 * @param size pointer where the result gets placed
 *
 * Checks the system capabilities for hugepages. And saves the maximum page
 * size for hugepages into @p size.
 *
 * @retval ERROR_IO_PROC_MEMINFO
 * @retval ERROR_NONE
 *
 */
error_t get_hugepagesize(uint64_t *size);

/**
 * @brief Checks for root permissions
 * @return 1 if the program has root access and 0 otherwise
 *
 * Checks if the current process has root permissions.
 */
int has_root_access();

/**
 * @brief Returns the id of the core on which the current thread is running on
 *
 * @param core the id of the CPU core
 *
 * Tries to get the current CPU core on which is program is running.
 *
 * @retval ERROR_GET_CPU_CORE
 * @retval ERROR_NONE
 *
 */
error_t get_current_cpu_core(uint32_t *core);

/**
 * @brief Returns the current status of the rdpmc command
 *
 * @retval ERROR_RDPMC
 * @retval ERROR_IO_RDPMC
 * @retval ERROR_NONE
 */
error_t can_use_rdpmc();
