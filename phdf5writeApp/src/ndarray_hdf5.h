/*
 * ndarray_hdf5.h
 *
 *  Created on: 6 Jan 2012
 *      Author: up45
 */

#ifndef NDARRAY_HDF5_H_
#define NDARRAY_HDF5_H_

#include <cstdlib>
#include <iostream>
#include <string>

#include <hdf5.h>

#include "dimension.h"

class NDArray; //forward declaration

// Stream NDArrays to a HDF5 file
class NDArrayToHDF5 {

#ifdef H5_HAVE_PARALLEL
public:
    NDArrayToHDF5(MPI_Comm comm, MPI_Info info):
        mpi_comm(comm),mpi_info(info){}; // parallel constructor
private:
    MPI_Comm mpi_comm;
    MPI_Info mpi_info;
#endif

public:
    NDArrayToHDF5(){};
    virtual ~NDArrayToHDF5(){};

    // h5_configure(); // is this needed?
    int h5_open(const char *filename);
    int h5_write(NDArray &ndarray);
    int h5_close();

protected:
    // print error and debug messages by default
    virtual void msg(const char *msg,
                     bool error=false)
    { std::cout << "NDArrayToHDF5: " << msg << std::endl;};

private:
    // attribute dataset operations
    int attr_create(){};
    int attr_write(){};
    int attr_close(){};

    // performance dataset operations
    int perf_create(){};
    int perf_write(){};
    int perf_close(){};

    // convenience function to write a HDF5 attribute as a string
    int write_h5attr_str(hid_t element,
                         std::string &attr_name,
                         std::string &str_attr_value){};

    // Write the HDF5 dataset attributes that makes the file NeXuS compatible
    int write_h5attr_nxs(){};

    //===== properties and attributes regarding file and dataset access =======

    // file access
    hsize_t alignment;
    unsigned int istorek;

    //===== HDF5 library handles ===============================================
    hid_t h5file;
};

#endif /* NDARRAY_HDF5_H_ */
