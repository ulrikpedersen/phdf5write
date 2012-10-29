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
#include "layoutxml.h"
#include "layout.h"
#include "writeconfig.hpp"
#include "profiling.h"
#include <NDArray.h>

#define NUM_WRITE_STEPS 2



// Stream NDArrays to a HDF5 file
class NDArrayToHDF5 {

#ifdef H5_HAVE_PARALLEL
public:
    NDArrayToHDF5(MPI_Comm comm, MPI_Info info); // parallel constructor
private:
    MPI_Comm mpi_comm;
    MPI_Info mpi_info;
#endif

public:
    NDArrayToHDF5();
    virtual ~NDArrayToHDF5(){this->h5_close();};

    int load_layout_xml();
    int load_layout_xml(std::string& xmlfile);
    int load_layout_xml(const char * xmlfile);

    void h5_configure(NDArray &ndarray);
    int h5_open(const char *filename);
    int h5_write(NDArray &ndarray);
    int h5_close();
    WriteConfig get_conf();
    WriteConfig& get_conf_ref();

    PHDF_DataType_t from_ndarr_to_phdf_datatype(NDDataType_t in) const;
    PHDF_DataType_t from_ndattr_to_phdf_datatype(NDAttrDataType_t in) const;
    PHDF_DataType_t from_hid_to_phdf_datatype(hid_t in) const;
    hid_t from_ndarr_to_hid_datatype(NDDataType_t in) const;
    hid_t from_phdf_to_hid_datatype(PHDF_DataType_t in) const;


protected:
    // print error and debug messages by default
    virtual void msg(std::string text, bool error=false)
        { msg(text.c_str(), error);};
    virtual void msg(const char *text,
                     bool error=false)
        { std::cout << "NDArrayToHDF5: " << text << std::endl;};

private:
    int mpi_rank;
    int mpi_size;
    int create_file_layout();
    int create_dataset(hid_t group, HdfDataset* dset);
    int _create_dataset_detector(hid_t group, HdfDataset* dset);
    int _create_dataset_metadata(hid_t group, HdfDataset* dset);

    int create_tree(HdfGroup *root, hid_t h5handle);
    void configure_ndattr_dsets(NDAttributeList *pAttributeList);

    // attribute dataset operations
    //int attr_create(){};
    //int attr_write(){};
    //int attr_close(){};

    // performance dataset operations
    int store_profiling();

    int write_hdf_attributes( hid_t h5dataset,  HdfElement* element);
    // convenience function to write a HDF5 attribute as a string
    /*
    int write_h5attr_str(hid_t element,
                         std::string &attr_name,
                         std::string &str_attr_value){};
    */

    // Write the HDF5 dataset attributes that makes the file NeXuS compatible
    //int write_h5attr_nxs(){};

    Profiling timestamp;
    Profiling dt_write;
    Profiling opentime;
    Profiling closetime;
    Profiling writestep[NUM_WRITE_STEPS];

    //===== properties and attributes regarding file and dataset access =======
    WriteConfig conf;
    LayoutXML layout;

    //===== HDF5 library handles ===============================================
    hid_t h5file;
    size_t rdcc_nslots;
    size_t rdcc_nbytes;
};

#endif /* NDARRAY_HDF5_H_ */
