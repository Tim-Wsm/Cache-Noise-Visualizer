/**
 * @file alloc.h
 * @date 18 Dec 2017
 *
 * @brief Contains functions to allocate and deallocate cache aligned buffers
 */
#pragma once

#include "error.h"
#include "sys_info.h"

#include <stdint.h>

/**
 * @brief allocates an aligned buffer.
 *
 * @param cache points to information about the cache which the buffer
 * corresponds to
 * @param buffer pointer a value that will contain a pointer to a buffer the
 * size and alignment of the given cache
 *
 * Allocates a buffer, where the first virtual address maps to the first cache
 * set and where all physical addresses are consecutive.
 *
 * @retval ERROR_ALLOCATION
 * @retval ERROR_SYSCONF
 * @retval ERROR_IO_PROC_MEMINFO
 * @retval ERROR_IO_PROC_SELF_PAGEMAP
 * @retval ERROR_FMT
 * @retval ERROR_NONE
 *
 */
error_t alloc_aligned(void **buffer, const cache_info_t *cache);

/**
 * @brief Frees a buffer created by alloc_aligned.
 *
 * @param cache points to information about the cache which the buffer
 * corresponds to
 * @param buffer pointer to the buffer that will be freed
 *
 * Frees a buffer created by alloc_aligned by calling munmap with the size of
 * the buffer plus the rest missing to one full hugepage.
 *
 * @retval ERROR_IO
 * @retval ERROR_NONE
 *
 */
error_t free_aligned(void *buffer, const cache_info_t *cache);
