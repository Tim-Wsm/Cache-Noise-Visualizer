/**
 * @file alloc.c
 * @date 18 Dec 2017
 *
 * @brief Contains all functions to allocate and deallocate cache aligned
 * buffers
 */

#include "alloc.h"

#include <stdlib.h>
#include <sys/mman.h>

/**
 * @brief tests if a buffer is aligned to the cache
 * @param cache points to information about the cache which the buffer
 * corresponds to
 * @param buffer pointer to the buffer that will be tested
 *
 * @retval ERROR_NOT_ALIGEND
 * @retval ERROR_IO_PROC_SELF_PAGEMAP
 * @retval ERROR_SYSCONF
 * @retval ERROR_NONE
 *
 * Tests if the buffer is aligned to the cache, by testing if every part of the
 * buffer corresponds to the correct cache line. This is archived by looking up
 * the physical address of every first virtual address of a (huge)page and
 * calculating to which cache line it belongs.
 */
error_t test_alignment(const void *buffer, const cache_info_t *info,
                       uint64_t page_size) {
    for (uint64_t current_line = 0;
         current_line < info->total_size / info->line_size;
         current_line += page_size / info->line_size) {
        void *ptr =
            (void *)(((uintptr_t)buffer) + current_line * info->line_size);

        // ensures that ptr is in memory
        *((int *)ptr) = 0;

        uintptr_t physical_address = 0;
        FORWARD_ON_FAIL(get_physical_address(ptr, &physical_address));

        // id of the cache line the pointer points to
        uintptr_t line_id =
            (physical_address % (info->total_size)) / info->line_size;

        if (current_line != line_id) {
            return ERROR_NOT_ALIGNED;
        }
    }

    return ERROR_NONE;
}

error_t alloc_aligned(void **buffer, const cache_info_t *cache) {
    uint32_t max_tries;
    FORWARD_ON_FAIL(get_hugepagenr(&max_tries));
    if (max_tries < 1) {
        return ERROR_NO_HUGEPAGES;
    }

    uint64_t hugepagesize = 0;
    FORWARD_ON_FAIL(get_hugepagesize(&hugepagesize));

    void **old_buffers = malloc(sizeof(void *) * max_tries);
    if (old_buffers == NULL) {
        return ERROR_ALLOCATION;
    }

    uint64_t tries = 0;
    error_t err = ERROR_NONE;
    for (; tries < max_tries; tries++) {
        *buffer = mmap(NULL, cache->total_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

        if ((*buffer) == MAP_FAILED) {
            err = ERROR_MMAP;
            break;
        }

        if ((err = test_alignment(*buffer, cache, hugepagesize)) ==
            ERROR_NOT_ALIGNED) {
            old_buffers[tries] = *buffer;
        } else {
            break;
        }
    }

    for (int i = 0; i < tries; i++) {
        err = free_aligned(old_buffers[i], cache);
    }
    free(old_buffers);

    if (tries < max_tries && err == ERROR_NONE) {
        printf("Found buffer after %lu tries.\n", tries);
    } else {
        if ((*buffer) != MAP_FAILED) {
            FORWARD_ON_FAIL(free_aligned(*buffer, cache));
        }
        fprintf(stderr, "Did not find buffer after %lu tries.\n", tries);
    }

    return err;
}

error_t free_aligned(void *buffer, const cache_info_t *cache) {
    // munmap does not unmap hugepages
    uint64_t hugepagesize;
    FORWARD_ON_FAIL(get_hugepagesize(&hugepagesize));

    size_t length = hugepagesize;
    while (length < cache->total_size) {
        length += hugepagesize;
    }

    if (munmap(buffer, length) == -1) {
        return ERROR_MUNMAP;
    }
    return ERROR_NONE;
}
