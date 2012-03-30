/*
 * ndarray_hdf5.cpp
 *
 *  Created on: 6 Jan 2012
 *      Author: up45
 */

#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

#include "ndarray_hdf5.h"

void print_arr (const char * msg, const hsize_t* sizes, size_t n);


NDArrayToHDF5::NDArrayToHDF5() {
    // load the default HDF5 layout
    this->mpi_rank = 0;
    this->mpi_size = 1;
    this->load_layout_xml();
}

#ifdef H5_HAVE_PARALLEL
NDArrayToHDF5::NDArrayToHDF5( MPI_Comm comm, MPI_Info info)
: mpi_comm(comm),mpi_info(info)
{
    this->load_layout_xml();
    MPI_Comm_size(comm,&this->mpi_size);
    MPI_Comm_rank(comm,&this->mpi_rank);

}
#endif

/** Load the default XML layout configuration
 * as defined in the 'layout.xml' file
 * TODO: currently the file must be present in current dir which is not good.
 */
int NDArrayToHDF5::load_layout_xml() {
    return this->load_layout_xml("layout.xml");
}

int NDArrayToHDF5::load_layout_xml(string& xmlfile) {
    return this->load_layout_xml( xmlfile.c_str() );
}

int NDArrayToHDF5::load_layout_xml(const char *xmlfile) {
    msg("h5_load_layout_xml()");
    return this->layout.load_xml(xmlfile);
}

void NDArrayToHDF5::h5_configure(NDArray& ndarray)
{
    msg("h5_configure()");

    int mpi_rank = 0, mpi_size=0;
#ifdef H5_HAVE_PARALLEL
    MPI_Comm_rank(this->mpi_comm,&mpi_rank);
    MPI_Comm_size(this->mpi_comm,&mpi_size);
#endif
    this->conf = WriteConfig(ndarray, mpi_rank, mpi_size);

    NDArrayInfo_t ndarrinfo;
    ndarray.getInfo(&ndarrinfo);
    cout << "ndarr totalBytes=" << ndarrinfo.totalBytes << endl;
    this->timestamp.reset(ndarrinfo.totalBytes);
    this->dt_write.reset(ndarrinfo.totalBytes);

    // Configure the chunk cache for datasets
    DimensionDesc chunk_cache = this->conf.min_chunk_cache();
    this->rdcc_nslots = (size_t)this->conf.cache_num_slots(chunk_cache);
    this->rdcc_nbytes = chunk_cache.data_num_bytes();
    cout << "Chunk cache: " << chunk_cache << endl;
    cout << "  - Number of slots in cache: " << this->rdcc_nslots << endl;
    cout << "  - Number of bytes in cache: " << this->rdcc_nbytes << endl;
}

int NDArrayToHDF5::h5_open(const char *filename)
{
    //msg("h5_open()");
    int retcode = 0;
    this->conf.file_name(filename);

    herr_t hdfcode;
    hid_t file_access_plist = H5Pcreate(H5P_FILE_ACCESS);

#ifdef H5_HAVE_PARALLEL
    msg("--------- HURRAH! WE ARE PARALLEL! ---------");
    int flag = 0;
    MPI_Initialized(&flag);
    if (flag != true) {
        msg("---- but sadly MPI has *not* been initialized!");

    } else {
        msg("---- and MPI has been initialized!");

        // Configure the file access property list to use MPI communicator
        if (this->conf.is_io_mpiposix()) {
            msg(" Configuring I/O: MPI + posix");
            hdfcode = H5Pset_fapl_mpiposix( file_access_plist, this->mpi_comm, 0 );
        } else {
            msg(" Configuring I/O: MPI-I/O");
            hdfcode = H5Pset_fapl_mpio( file_access_plist, this->mpi_comm, this->mpi_info );
        }
         if (hdfcode < 0) {
            msg("ERROR: failed to set MPI communicator", true);
            H5Pclose(file_access_plist);
            return -1;
        }

    }
#endif

    // Disabling of the meta data cache is mentioned in various places as
    // a performance tweak. However for our data-model it does not appear to
    // have any significant impact.
/*
    cout << "Disabling meta data cache" <<endl;
    H5AC_cache_config_t mdc_config;
    mdc_config.version = H5AC__CURR_CACHE_CONFIG_VERSION;
    hdfcode = H5Pget_mdc_config(file_access_plist, &mdc_config);
    if (hdfcode < 0) {
        msg("Warning: failed to get mdc_config for disabling metadata cache");
    } else {
        mdc_config.evictions_enabled = 0;
        mdc_config.incr_mode = H5C_incr__off;
        mdc_config.decr_mode = H5C_decr__off;
        mdc_config.flash_incr_mode = H5C_flash_incr__off;
        hdfcode = H5Pset_mdc_config(file_access_plist, &mdc_config);
        if (hdfcode < 0) {
            msg("Warning: failed to set mdc_config to disable metadata cache");
        }
    }
    */

    HSIZE_T alignment = this->conf.get_alignment();
    cout << "Setting file alignment: " << alignment << endl;
    hdfcode = H5Pset_alignment( file_access_plist, alignment, alignment);
    if (hdfcode < 0) {
        msg("Warning: failed to set alignement");
    }
    long int istorek = this->conf.istorek();
    cout << "Setting istorek: " << istorek << endl;
    hid_t create_plist = H5Pcreate(H5P_FILE_CREATE);
    hdfcode = H5Pset_istore_k(create_plist, istorek);
    if (hdfcode < 0) {
        msg("Warning: failed to set i_store_k");
    }

    opentime.reset();
    this->h5file = H5Fcreate( filename, H5F_ACC_TRUNC, create_plist, file_access_plist);
    if (this->h5file == H5I_INVALID_HID) {
        msg("ERROR: unable to open/create file", true);
        H5Pclose(file_access_plist);
        H5Pclose(create_plist);
        return -1;
    }


    H5Pclose(file_access_plist);
    H5Pclose(create_plist);
    opentime.stamp_now();
    msg(opentime._str().c_str());

    // create the HDF5 file structure: groups and datasets
    retcode = this->create_file_layout();

    return retcode;
}

int NDArrayToHDF5::h5_write(NDArray& ndarray)
{
    int retcode = 0;
    herr_t hdferr = 0;
    //msg("h5_write()");

    // Initialise performance measurements
    this->writestep[0].dt_start();
    timespec startstamp = this->writestep[0].get_start();
    for (int i=1; i<NUM_WRITE_STEPS; i++)
    {
        this->writestep[i].dt_set_startstamp( startstamp );
    }

    this->conf.next_frame(ndarray);
    //msg(this->conf._str_());

    // Timing configuration step
    //this->writestep[0].dt_end(); // takes no measurable time

    hid_t dset_access_plist = H5Pcreate(H5P_DATASET_ACCESS);
    hdferr = H5Pset_chunk_cache(dset_access_plist, this->rdcc_nslots, this->rdcc_nbytes, 1.0);
    // Timing caching configuration
    //this->writestep[1].dt_end(); // takes no measurable time

    const char *dset_name = "/mydset";
    hid_t dataset = H5Dopen2(this->h5file, dset_name, dset_access_plist);
    if (dataset < 0) {
        msg("ERROR: unable to open dataset", true);
        return -1;
    }
    // Timing opening of dataset
    //this->writestep[2].dt_end(); // takes no measurable time

    if (this->conf.is_dset_extendible()) {
        vec_ds_t dset_vec = this->conf.get_dset_dims();
        const hsize_t *dset_ptr = WriteConfig::get_vec_ptr(dset_vec);
        print_arr("Extending to: ", dset_ptr, dset_vec.size());
        hdferr = H5Dset_extent( dataset, dset_ptr );
        if (hdferr < 0) {
            msg("ERROR: unable to extend dataset", true);
            if (dset_access_plist > 0) H5Pclose(dset_access_plist);
            H5Dclose(dataset);
            return -1;
        }
    }
    // Timing extending dataset
    this->writestep[0].dt_end();


    hid_t file_dataspace = H5Dget_space(dataset);
    if (file_dataspace < 0) {
        msg("ERROR: unable to get dataspace", true);
        if (dset_access_plist > 0) H5Pclose(dset_access_plist);
        H5Dclose(dataset);
        return -1;
    }
    // Timing getting the file dataspace description
    //this->writestep[1].dt_end(); // takes no measurable time


    hid_t datatype = H5Dget_type(dataset);
    if (datatype < 0) {
        msg("ERROR: unable to get datatype", true);
        H5Sclose(file_dataspace);
        if (dset_access_plist > 0) H5Pclose(dset_access_plist);
        H5Dclose(dataset);
        return -1;
    }
    // Timing getting the file dataspace datatype
    //this->writestep[2].dt_end(); // takes no measurable time

    vec_ds_t offset_vec =  this->conf.get_offsets();
    const hsize_t *offset_ptr = WriteConfig::get_vec_ptr( offset_vec );
    //print_arr("Offsets: ", offset_ptr, offset_vec.size());

    vec_ds_t roi_fr_vec =  this->conf.get_roi_frame();
    // TODO: This is a hack: We are forcing an extra dimension on the copy of the ROI.
    roi_fr_vec.insert(roi_fr_vec.begin(), 1);

    const hsize_t *roi_fr_ptr = WriteConfig::get_vec_ptr( roi_fr_vec );
    //print_arr("ROI: ", roi_fr_ptr, roi_fr_vec.size());

    // Timing getting the dimension sizes and offsets
    //this->writestep[3].dt_end();// takes no measurable time

    hdferr = H5Sselect_hyperslab( file_dataspace, H5S_SELECT_SET,
                                         offset_ptr, NULL,
                                         roi_fr_ptr, NULL);
    if (hdferr < 0) {
        msg("ERROR: unable to select hyperslab", true);
        H5Sclose(file_dataspace);
        H5Tclose(datatype);
        if (dset_access_plist > 0) H5Pclose(dset_access_plist);
        H5Dclose(dataset);
        return -1;
    }
    // Timing selecting hyperslab in file space
    //this->writestep[4].dt_end();// takes no measurable time


    // Memory data_space
    hid_t mem_dataspace = H5Screate_simple(roi_fr_vec.size(), roi_fr_ptr, NULL);
    if (mem_dataspace < 0) {
        msg("ERROR: unable to create memory dataspace", true);
        H5Sclose(file_dataspace);
        H5Tclose(datatype);
        if (dset_access_plist > 0) H5Pclose(dset_access_plist);
        H5Dclose(dataset);
        return -1;
    }
    // Timing creating a memory dataspace
    //this->writestep[5].dt_end();// takes no measurable time


    // Transfer property list to select collective or independent transfer
    hid_t xfer_plist = H5P_DEFAULT;
#ifdef H5_HAVE_PARALLEL
    xfer_plist = H5Pcreate(H5P_DATASET_XFER);
    if (xfer_plist < 0) {
        msg("ERROR: unable to create transfer plist", true);
    } else if (not this->conf.is_io_mpiposix()){
        H5FD_mpio_xfer_t xfermode;
        xfermode = (this->conf.is_io_collective() ? H5FD_MPIO_COLLECTIVE : H5FD_MPIO_INDEPENDENT);
        hdferr = H5Pset_dxpl_mpio(xfer_plist, xfermode);
        if (hdferr < 0)
        {
            msg("ERROR: unable to configure transfer with collective I/O", true);
        }
    }
#endif
    // Timing setting the MPIO transfer method
    //this->writestep[6].dt_end();// takes no measurable time

    // The dataset, datatype and dataspace is not correct here...
    this->dt_write.dt_start();
    hdferr = H5Dwrite( dataset, datatype, mem_dataspace,
                       file_dataspace, xfer_plist, ndarray.pData);
    if (hdferr < 0) {
        msg("ERROR: unable to write to dataset", true);
        H5Sclose(mem_dataspace);
        H5Sclose(file_dataspace);
        H5Tclose(datatype);
        if (dset_access_plist > 0) H5Pclose(dset_access_plist);
        H5Dclose(dataset);
        this->dt_write.dt_end();
        return -1;
    }
    this->dt_write.dt_end();
    // Timing writing the data
    this->writestep[1].dt_end();

    H5Sclose(mem_dataspace);
    H5Sclose(file_dataspace);
    H5Tclose(datatype);
    //H5Pclose(dapl_id);
    H5Pclose(dset_access_plist);
    H5Dclose(dataset);
    this->timestamp.stamp_now();
    // Timing closing the handles
    //this->writestep[8].dt_end();// takes no measurable time
    //cout << this->writestep[0] << endl;
    //cout << this->writestep[11] << endl;

    return retcode;
}

int NDArrayToHDF5::h5_close()
{
    //msg("h5_close()");
    int retcode = 0;
    herr_t hdferr = 0;
    char *fname = new char(512);

    if (this->h5file != H5I_INVALID_HID) {
        //msg("Writing profile data");
        this->store_profiling();
        H5Fget_name(this->h5file, fname, 512-1 );
        cout << "Closing file: " << fname << endl;
        closetime.reset();
        hdferr = H5Fclose(this->h5file);
        closetime.stamp_now();
        cout << "File close time: " << closetime << endl;
        this->h5file = H5I_INVALID_HID;
        if (hdferr < 0) {
            cerr << "ERROR: Failed to close file" << endl;
            retcode = -1;
        }
    }

    return retcode;
}

WriteConfig NDArrayToHDF5::get_conf()
{
    return this->conf;
}

WriteConfig& NDArrayToHDF5::get_conf_ref()
{
    return this->conf;
}


/*============== NDArrayToHDF5 private method implementations ================*/

/** Create the groups and datasets in the HDF5 file.
 * TODO: currently only creates the main dataset. Need to run through all defined datasets and create them in the file.
 */
int NDArrayToHDF5::create_file_layout()
{
    int retcode = 0;

    HdfGroup *tree = this->layout.get_hdftree();

    // for the moment we just create the main dataset (with the attribute 'signal')
    string signal("signal");
    HdfDataset *dset = NULL;
    retcode = tree->find_dset_ndattr(signal, &dset);
    if (retcode < 0) return retcode;
    if (dset == NULL) return -1;

    retcode = this->create_dataset(dset);

    return retcode;
}

// utility method to help during debugging
void print_arr (const char * msg, const hsize_t* sizes, size_t n)
{
    cout << msg << " [ ";
    for (unsigned int i=0; i<n; i++)
    {
        cout << *(sizes+i) << ", ";
    }
    cout << "]" << endl;
}

/** Create a dataset in the HDF5 file with the details defined in the dset argument.
 * Return 0 on success, negative value on error. Errors: fail to set chunk size or
 * failure to create the dataset in the file.
 */
int NDArrayToHDF5::create_dataset(HdfDataset *dset)
{
    int retcode = 0;
    if (dset == NULL) return -1; // sanity check
    herr_t hdfcode;
    hid_t dset_create_plist = -1;
    hid_t dataspace = -1;
    hid_t dataset = -1;

    dset_create_plist = H5Pcreate(H5P_DATASET_CREATE);
    // Set the chunk size as defined in the configuration
    vec_ds_t chunk_dims = this->conf.get_chunk_dims();

    const hsize_t *chunk_dims_ptr = WriteConfig::get_vec_ptr(chunk_dims);
    print_arr("chunk dims ptr: ", chunk_dims_ptr, chunk_dims.size());
    hdfcode = H5Pset_chunk( dset_create_plist,
                            chunk_dims.size(),
                            chunk_dims_ptr);
    if (hdfcode < 0) {
        cerr << "ERROR: Failed to set chunksize "<< endl;
        return -1;
    }

    // TODO: hardcoded datatype of 16bit int is not good!
    //hid_t datatype = H5T_NATIVE_UINT_FAST16;
    hid_t datatype = H5T_NATIVE_UINT16;

    // TODO: configure compression if requred (although not for phdf5 as this would not work)

    // Setting the fill value
    // TODO: fix hardcoded 16bit fill value
    unsigned short fillvalue = 0;
    size_t size_fillvalue = sizeof(unsigned short);
    this->conf.get_fill_value((void*)&fillvalue, &size_fillvalue);
    size_t fillvaluesize = sizeof(unsigned short);
    this->conf.get_fill_value((void*)&fillvalue, &fillvaluesize);
    hdfcode = H5Pset_fill_value(dset_create_plist, datatype, (void*)&fillvalue);
    if (hdfcode < 0) {
        cerr << "Warning: Failed to set fill value" << endl;
        //return hdfcode;
    }


    // Configure the dataset dimensions and max dimensions
    vec_ds_t vec_maxdim = this->conf.get_dset_maxdims();
    const hsize_t *ptr_maxdims = WriteConfig::get_vec_ptr(vec_maxdim);
    print_arr("max dim: ", ptr_maxdims, vec_maxdim.size());
    vec_ds_t vec_dset = this->conf.get_dset_dims();
    const hsize_t *ptr_dset_dim =  WriteConfig::get_vec_ptr(vec_dset);
    if (not this->conf.is_dset_extendible())
    {
        ptr_dset_dim = ptr_maxdims;
    }
    print_arr("dataset ptr: ", ptr_dset_dim, vec_dset.size());
    dataspace = H5Screate_simple( (int)this->conf.get_dset_dims().size(),
                                  ptr_dset_dim, ptr_maxdims);
    if (dataspace < 0) {
        cerr << "ERROR: failed to create dataspace for dataset: " << dset->get_full_name() << endl;
        if (dset_create_plist > 0) H5Pclose(dset_create_plist);
        return -1;
    }

    //const char * dsetname = dset->get_full_name().c_str();
    const char * dsetname = "mydset";
    cout << "Creating dataset: " << dsetname << endl;
    dataset = H5Dcreate2( this->h5file, dsetname,
                                datatype, dataspace,
                                H5P_DEFAULT, dset_create_plist, H5P_DEFAULT);
    if (dataset < 0) {
        cerr << "ERROR: Failed to create dataset: " << dset->get_full_name() << endl;
        retcode = -1;
        if (dataspace > 0) H5Sclose(dataspace);
        if (dset_create_plist > 0) H5Pclose(dset_create_plist);
    }

    hdfcode = this->write_h5dataset_attributes( dataset,  dset);
    if (hdfcode < 0) {
        cerr << "Warning: Failed to write dataset attributes on: " << dset->get_full_name() << endl;
    }

    if (dataset > 0) H5Dclose(dataset);
    if (dataspace > 0) H5Sclose(dataspace);
    if (dset_create_plist > 0) H5Pclose(dset_create_plist);
    return retcode;
}

/** Translate the NDArray datatype to HDF5 datatypes */
hid_t NDArrayToHDF5::type_nd2hdf(NDDataType_t& datatype)
{
  hid_t result;
  switch (datatype) {
    case NDInt8:
      result = H5T_NATIVE_INT_FAST8;
      break;
    case NDUInt8:
      result = H5T_NATIVE_UINT_FAST8;
      break;
    case NDInt16:
      result = H5T_NATIVE_INT_FAST16;
      break;
    case NDUInt16:
      result = H5T_NATIVE_UINT_FAST16;
      break;
    case NDInt32:
      result = H5T_NATIVE_INT_FAST32;
      break;
    case NDUInt32:
      result = H5T_NATIVE_UINT_FAST32;
      break;
    case NDFloat32:
      result = H5T_NATIVE_FLOAT;
      break;
    case NDFloat64:
      result = H5T_NATIVE_DOUBLE;
      break;
    default:
      cerr << "HDArrayToHDF5: cannot convert NDArrayType: "<< datatype << " to HDF5 datatype" << endl;
      result = -1;
  }
  return result;
}

/** Store the profile data in a separate dataset
 *
 */
int NDArrayToHDF5::store_profiling()
{
    int retcode = 0;
    hid_t dataspace;
    hid_t dataset;
    herr_t hdferr;
    const int ndims = 3;

    // For each profiling object, write to the correct column...
    vector< vector<double> > profs;
    profs.push_back(this->timestamp.vec_timestamps());
    profs.push_back(this->timestamp.vec_deltatime());
    profs.push_back(this->dt_write.vec_timestamps());
    profs.push_back(this->timestamp.vec_datarate());
    profs.push_back(this->dt_write.vec_datarate());
    for (int i = 0; i<NUM_WRITE_STEPS; i++) profs.push_back(this->writestep[i].vec_timestamps());

    // Find the longest profiling dataset (they should all be same length)
    vector< vector<double> >::const_iterator it;
    hsize_t maxlen = 0;
    for (it = profs.begin(); it != profs.end(); ++it)
    {
        if ((*it).size() > maxlen) maxlen = (*it).size();
    }

    hsize_t profiling_dims[ndims] = {this->mpi_size, profs.size(), maxlen};

    dataspace = H5Screate_simple( ndims, profiling_dims, NULL);
    if (dataspace < 0) {
        cerr << "ERROR: failed to create profiling dataspace" << endl;
        return -1;
    }

    /* todo: fix hardcoded profiling dataset name */
    const char * dsetname = "profiling";
    cout << "Creating dataset: " << dsetname << endl;
    dataset = H5Dcreate2( this->h5file, dsetname,
                          H5T_NATIVE_DOUBLE, dataspace,
                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dataset < 0) {
        cerr << "ERROR: Profiling: Failed to create dataset: " << dsetname << endl;
        if (dataspace > 0) H5Sclose(dataspace);
        return -1;
    }

    hid_t file_dataspace = H5Dget_space(dataset);

    int i = 0;
    for (it = profs.begin(); it != profs.end(); ++it, i++)
    {
        vector<double> pf = *it;
        hsize_t start[ndims] = { this->mpi_rank, i, 0 };
        hsize_t count[ndims] = { 1, 1, pf.size() };
        hdferr = H5Sselect_hyperslab( file_dataspace, H5S_SELECT_SET,
                                      start, NULL,
                                      count, NULL);
        hid_t mem_dataspace = H5Screate_simple(ndims, count, NULL);
        if (mem_dataspace < 0) {
            msg("ERROR: Profiling: unable to create memory dataspace", true);
            H5Sclose(file_dataspace);
            H5Dclose(dataset);
            return -1;
        }
        hdferr = H5Dwrite( dataset, H5T_NATIVE_DOUBLE, mem_dataspace,
                           file_dataspace, H5P_DEFAULT, &pf.front() );
        if (hdferr < 0) {
            msg("ERROR: Profiling: unable to write to dataset", true);
            H5Sclose(mem_dataspace);
            H5Sclose(file_dataspace);
            H5Dclose(dataset);
            return -1;
        }
        H5Sclose(mem_dataspace);
    }


    if (dataspace > 0) H5Sclose(dataspace);
    H5Sclose(file_dataspace);
    H5Dclose(dataset);

    return retcode;
}

// TODO: write attributes to dataset
int NDArrayToHDF5::write_h5dataset_attributes( hid_t h5dataset, HdfDataset* dset)
{
    int retcode = -1;



    return retcode;
}
