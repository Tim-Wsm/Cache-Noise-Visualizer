/**
 * @file output.h
 * @date 13 Mar 2017
 *
 * @brief Contains functions to handle HDF5 files.
 */

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "hdf5.h"

#include "error.h"

#define OUTPUT_STDOUT 1
#define OUTPUT_HD5_FILE 2

typedef struct output_s {
    uint8_t type;
    FILE *std;
    hid_t h5;
    uintptr_t iter;
} output_t;

/**
 * @brief Prints a matrix which is one dimensional in the memory and
 * holds i32 values.
 *
 * @param output Holds data about the output stream
 * @param data Matrix
 * @param dimX dimension (x-axis)
 * @param dimY dimension (y-axis)
 *
 * Writes the output to the output stream which is specified in output_t. The
 * data is an two dimensional matrix which has an x and y dimension.
 *
 * @retval ERROR_IO_HDF
 * @retval ERROR_HDF5_ERROR
 * @retval ERROR_NOT_SUPPORTED_OUTPUT
 * @retval ERROR_NONE
 *
 */
error_t outputw_mat_ui32(output_t *output, uint32_t *data, uintptr_t dim_x,
                         uintptr_t dim_y);

/**
 * @brief Creates a new output_t with an FILE as output.
 *
 * @param output Holds data about the output stream
 * @param file An file descriptor.
 *
 * There can be any kind of files or std-streams.
 *
 * @retval ERROR_NONE
 *
 */
error_t outputc_stdout(output_t *output, FILE *file);

/**
 * @brief Creates a new output_t with an HDF5 as output.
 *
 * @param output Holds data about the output stream
 * @param file An file name to a new HDF5 file.
 *
 * The file do not exist and will be created.
 *
 * @retval ERROR_HDF5_ERROR
 * @retval ERROR_CHMOD
 * @retval ERROR_NONE
 *
 */
error_t outputc_hd5_file(output_t *output, char *file);

/**
 * @breif Closes the streams and flushes them.
 *
 * @param output Holds data about the output stream
 *
 * After this operation the output_t struct has to be initialized again.
 *
 * @retval ERROR_HDF5_ERROR
 * @retval ERROR_NONE
 *
 */
error_t output_close(output_t *output);
