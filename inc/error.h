/**
 * @file error.h
 * @date 9 Nov 2017
 *
 * @brief Contains functions and definitions for error handling.
 */
#pragma once

#include <errno.h>
#include <string.h>

/**
 * @brief IO error occurred.
 * If any kind of IO occurs then this kind of error is returned.
 * The error of the IO error can be read via errno.
 */
#define ERROR_IO -1

/**
 * @brief malloc/calloc failed
 *
 * This error is returned if the program could not allocate memory.
 */
#define ERROR_ALLOCATION -2

/**
 * @brief File descriptor creation failed (for cache misses)
 *
 * This error is returned if no file descriptor with the cache misses could
 * be initialized.
 */
#define ERROR_FD_CACHE -4

/**
 * @brief File descriptor creation failed (for CPU cycles)
 *
 * This error is returned if no file descriptor with the cycle duration could
 * be initialized.
 */
#define ERROR_FD_CYCLE -5

/**
 * @brief Failed to set the affinity of a process.
 *
 * The kernel could not set the affinity for one process to one CPU core.
 */
#define ERROR_SET_AFFINITY -6

/**
 * @brief Failed to read formatted input.
 *
 * This error occurs if e.g. scanf could not read the input properly.
 */
#define ERROR_FMT -7

/**
 * @brief An error occurred while getting a sysconf variable.
 *
 * If the program tires to get a system variable but the variable is
 * not valid, then this error is returned. Or if the name of the
 * variable is invalid.
 */
#define ERROR_SYSCONF -8

/**
 * @brief The given address is not present in the ram.
 *
 * The page is not present in the ram. If it's not in
 * the ram then the offset is calculated differently.
 * Other functions than are not implemented.
 */
#define ERROR_PAGE_ENTRY -9

/**
 * @brief The given buffer is not aligned to the cache.
 *
 * The first virtual address of the given buffer does not map onto the first
 * cache set.
 */
#define ERROR_NOT_ALIGNED -10

/**
 * @brief An error occurred while using mmap.
 *
 */

#define ERROR_MMAP -11

/**
 * @brief An error because the wanted cache does not exist
 *
 */
#define ERROR_CACHE_NOT_EXISTS -12

/**
 * @brief An error because there are no hugepages available on the system
 *
 */
#define ERROR_NO_HUGEPAGES -13

/**
 * @brief An error because munmap failed
 *
 */
#define ERROR_MUNMAP -14

/**
 * @brief file descriptor destruction failed (for CPU cycles)
 *
 */
#define ERROR_FD_CYCLE_CLOSE -16

/**
 * @brief Could not initialize the attr attribute
 *
 */
#define ERROR_SPAWN_ATTR_INIT -17

/**
 * @brief Could not initialize the file attribute
 *
 */
#define ERROR_SPAWN_FILE_INIT -18

/**
 * @brief Error while spawning child
 *
 */
#define ERROR_SPAWN_SPAWN -19

/**
 * @brief The path to the program is null.
 *
 */
#define ERROR_SPAWN_PATH_NULL -20

/**
 * @brief The given output method is not supported.
 *
 */
#define ERROR_NOT_SUPPORTED_OUTPUT -21

/**
 * @brief One of the HDF5 functions returned an error.
 *
 */
#define ERROR_HDF5_ERROR -22

/**
 * @brief Failed to set the scheduler policy of the thread.
 *
 * The kernel could not set the scheduler policy for a thread.
 */
#define ERROR_SET_SCHEDULER_POLICY -21

/**
 * @brief Failed to retrive the CPU core on which the thread currently runs on.
 */
#define ERROR_GET_CPU_CORE -22

/**
 * @brief Failed to change chmod for the output file.
 */
#define ERROR_CHMOD -23

/**
 * @brief IO error occurred in /proc/self/pagemap
 */
#define ERROR_IO_PROC_SELF_PAGEMAP -24

/**
 * @brief There was an IO error in this file structure.
 */
#define ERROR_IO_SYS_CPU -25

/**
 * @brief IO error occurred in /proc/meminfo
 */
#define ERROR_IO_PROC_MEMINFO -26

/**
 * @brief IO error occurred working with the hdf file
 */
#define ERROR_IO_HDF -27

/**
 * @brief There was an IO error in this file structure.
 */
#define ERROR_IO_LINE_SIZE -28

/**
 * @brief There was an IO error in this file structure.
 */
#define ERROR_IO_TOTAL_SIZE -29

/**
 * @brief There was an IO error in this file structure.
 */
#define ERROR_IO_SET_NUMBER -30

/**
 * @brief There was an IO error in this file structure.
 */
#define ERROR_IO_LEVELS -32

/**
 * @brief There was an IO error in this file structure.
 */
#define ERROR_IO_ASSOCIATIVITY -33

/**
 * @brief There was an IO error in this file structure.
 */
#define ERROR_IO_HUGEPAGE_NUMBER -34

/**
 * @brief There was an IO while accessing
 * /sys/bus/event_source/devices/cpu/rdpmc
 */
#define ERROR_IO_RDPMC -35

/**
 * @brief The rdpmc instruction is not available in userspace
 */
#define ERROR_RDPMC -36

/**
 * @brief If the execution was successful
 *
 * This value is returned if no errors are returned.
 * It's the same as return 0 in normal programs.
 */
#define ERROR_NONE 0

#ifndef __error_t_defined
#define __error_t_defined 1
typedef int error_t;
#endif

/**
 * @brief Maps an error code to an error message.
 *
 * This functions combines an error code with an error message. If this is used
 * with an array this can be used as a lookup table for error codes.
 */
typedef struct error_unit_s {
    const error_t number;
    const char *message;
} error_unit_t;

/**
 * @brief Decodes the error defined in this file, into strings.
 *
 * @param error Error code from this file.
 * @return A constant string.
 */
const char *decode_error(error_t error);

/**
 * @brief Simplifies the error handling process in the main.c.
 *
 * @param error_code Any value which contains a valid error code.
 * @param message Custom message which is printed to the console.
 *
 * This function unwraps a return value of a supported function.
 * That means the function must return one of the values defined in
 * this file. Otherwise it will print an 'unknown error' error.
 * If the error code is something else than ERROR_NONE, the code
 * will be printed out in a human readable form and then
 * exit(EXIT_FAILURE) will be called.
 */
#define EXIT_ON_FAIL(error_code, message)                                      \
    {                                                                          \
        error_t res = error_code;                                              \
        if (res != ERROR_NONE) {                                               \
            fprintf(stderr, "%s, %s(%d) ", message, decode_error(error_code),  \
                    error_code);                                               \
            if (errno) {                                                       \
                fprintf(stderr, ":%s(%d)", strerror(errno), errno);            \
            }                                                                  \
            fprintf(stderr, "\n");                                             \
            goto FINALIZE;                                                     \
        }                                                                      \
    }

/**
 * @brief Simplifies the error handling process in functions that are not main
 * @param error_code Any value which contains a valid error code.
 *
 * This function unwraps a return value of a supported function.
 * That means the function must return one of the values defined in
 * this file. Otherwise it will print an 'unknown error' error.
 * If the error code is something else than ERROR_NONE, it will be returned
 * to the caller function.
 **/
#define FORWARD_ON_FAIL(error_code)                                            \
    {                                                                          \
        error_t res = error_code;                                              \
        if (res != ERROR_NONE) {                                               \
            return res;                                                        \
        }                                                                      \
    }
