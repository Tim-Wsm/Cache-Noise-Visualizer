/**
 * @file output.c
 * @date 13 Mar 2017
 *
 * @brief Contains functions to handle HDF5 files.
 */

#include "output.h"

error_t outputw_mat_ui32(output_t *output, uint32_t *data, uintptr_t dim_x,
                         uintptr_t dim_y) {
    if (output->type == OUTPUT_STDOUT) {
        for (uintptr_t set = 0; set < dim_y; set++) {
            if (fprintf(output->std, "set %lu: ", set) < 0) {
                return ERROR_IO_HDF;
            }

            if (fprintf(output->std, "%u", data[set * dim_x]) < 0) {
                return ERROR_IO_HDF;
            }

            for (uintptr_t way = 1; way < dim_x; way++) {
                if (fprintf(output->std, ",%u", data[set * dim_x + way]) < 0) {
                    return ERROR_IO_HDF;
                }
            }

            if (fprintf(output->std, "\n") < 0) {
                return ERROR_IO_HDF;
            }
        }

    } else if (output->type == OUTPUT_HD5_FILE) {
        hid_t dataset_id, dataspace_id;
        herr_t status;
        hsize_t dims[2];

        dims[0] = dim_y;
        dims[1] = dim_x;

        dataspace_id = H5Screate_simple(2, dims, NULL);

        char id[256];

        sprintf(id, "%lu", output->iter++);

        dataset_id = H5Dcreate(output->h5, id, H5T_STD_U32BE, dataspace_id,
                               H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        if (dataset_id == -1) {
            return ERROR_HDF5_ERROR;
        }

        status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                          H5P_DEFAULT, data);

        if (status == -1) {
            return ERROR_HDF5_ERROR;
        }

        status = H5Dclose(dataset_id);

        if (status == -1) {
            return ERROR_HDF5_ERROR;
        }
    } else {
        return ERROR_NOT_SUPPORTED_OUTPUT;
    }

    return ERROR_NONE;
}

error_t outputc_stdout(output_t *output, FILE *file) {
    output->h5 = -1;
    output->iter = 0;
    output->std = file;
    output->type = OUTPUT_STDOUT;

    return ERROR_NONE;
}

error_t outputc_hd5_file(output_t *output, char *file) {
    hid_t file_id;

    file_id = H5Fcreate(file, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    if (chmod(file, 0666)) {
        return ERROR_CHMOD;
    }

    if (file_id == -1) {
        return ERROR_HDF5_ERROR;
    }

    output->std = NULL;
    output->h5 = file_id;
    output->iter = 0;
    output->type = OUTPUT_HD5_FILE;

    return ERROR_NONE;
}

error_t output_close(output_t *output) {
    if (output->type == OUTPUT_HD5_FILE) {
        if (H5Fclose(output->h5) == -1) {
            return ERROR_HDF5_ERROR;
        }
    }

    return ERROR_NONE;
}
