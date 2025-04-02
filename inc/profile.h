/**
 * @file profile.h
 * @date 9 Nov 2017
 *
 * @brief Contains functions for profiling.
 */

#pragma once

#include "error.h"
#include "output.h"
#include "sys_info.h"

#include <stdint.h>

/**
 * @brief Profiles the cache and prints the result to the output.
 *
 * @param cache pointer to information about the cache which is profiled
 * @param cpu the bounded cpu id
 * @param output file descriptor of the file where the results will be printed
 * @param iterations the program will run for n iterations
 * @param buffer cache aligned buffer
 *
 * Allocates a buffer the size of the cache, which is aligned to the cache
 * lines. Then the time it takes to access memory from a single cache line is
 * measured and stored in a separate buffer. After all cache lines have been
 * accessed the result buffer gets printed to the output.
 *
 * @retval ERROR_ALLOCATION
 * @retval ERROR_SYSCONF
 * @retval ERROR_IO
 * @retval ERROR_FMT
 * @retval ERROR_FD_CYCLE
 * @retval ERROR_FD_CYCLE_CLOSE
 * @retval ERROR_HDF5_ERROR
 * @retval ERROR_NOT_SUPPORTED_OUTPUT
 * @retval ERROR_NONE
 *
 */
error_t profile(const cache_info_t *cache, uint32_t cpu, uint32_t iterations,
                const void *buffer, output_t *output);

/**
 * @brief benchmarks the accuracy of the measurement
 *
 * @param iterations specifies the number of times the measurements will be
 * repeated
 *
 * This benchmark measures the time of different performance timer.
 *
 * @retval ERROR_FD_CYCLE
 * @retval ERROR_FD_CYCLE_CLOSE
 * @retval ERROR_NONE
 *
 */
error_t benchmark(uint64_t iterations);
