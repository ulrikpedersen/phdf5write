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



#define METADATA_NDIMS 2

NDArrayToHDF5::NDArrayToHDF5()
: h5file(H5I_INVALID_HID),rdcc_nslots(0),rdcc_nbytes(0) {
	log = log4cxx::Logger::getLogger("NDArrayToHDF5");
#ifdef H5_HAVE_PARALLEL
	this->mpi_comm = 0;
	this->mpi_info = 0;
#endif
    // load the default HDF5 layout
    this->mpi_rank = 0;
    this->mpi_size = 1;
}

#ifdef H5_HAVE_PARALLEL
NDArrayToHDF5::NDArrayToHDF5( MPI_Comm comm, MPI_Info info)
: mpi_comm(comm),mpi_info(info),h5file(H5I_INVALID_HID),rdcc_nslots(0),rdcc_nbytes(0)
{
	log = log4cxx::Logger::getLogger("NDArrayToHDF5");
    MPI_Comm_size(comm,&this->mpi_size);
    MPI_Comm_rank(comm,&this->mpi_rank);

}
#endif

int NDArrayToHDF5::load_layout_xml(string& xmlfile) {
    return this->load_layout_xml( xmlfile.c_str() );
}

int NDArrayToHDF5::load_layout_xml(const char *xmlfile) {
    LOG4CXX_TRACE(log, "h5_load_layout_xml()");
    return this->layout.load_xml(xmlfile);
}

void NDArrayToHDF5::h5_configure(NDArray& ndarray)
{
    LOG4CXX_DEBUG(log, "h5_configure()");

    int mpi_rank = 0, mpi_size=0;
#ifdef H5_HAVE_PARALLEL
    MPI_Comm_rank(this->mpi_comm,&mpi_rank);
    MPI_Comm_size(this->mpi_comm,&mpi_size);
#endif
    this->conf = WriteConfig(ndarray, mpi_rank, mpi_size);

    NDArrayInfo_t ndarrinfo;
    ndarray.getInfo(&ndarrinfo);
    LOG4CXX_DEBUG(log, "ndarr totalBytes=" << ndarrinfo.totalBytes );
    this->timestamp.reset(ndarrinfo.totalBytes);
    this->dt_write.reset(ndarrinfo.totalBytes);

    // Configure the tree structure with the NDAttributes from this NDArray
    this->configure_ndattr_dsets(ndarray.pAttributeList);

    // Set the detector datatype based on the NDArray datatype
    HdfGroup::MapDatasets_t detector_dsets;
    HdfRoot * hdfTreeRoot = this->layout.get_hdftree();
    if (hdfTreeRoot == NULL) {
    	LOG4CXX_ERROR(log, "no hdf tree root configured this will end badly!");
    } else {
    	hdfTreeRoot->find_dsets(phdf_detector, detector_dsets);
    }
    HdfGroup::MapDatasets_t::iterator it_dsets;
    for (it_dsets = detector_dsets.begin(); it_dsets != detector_dsets.end(); ++it_dsets)
    {
    	PHDF_DataType_t dtype = NDArrayToHDF5::from_ndarr_to_phdf_datatype(ndarray.dataType);
    	HdfDataSource src(phdf_detector, dtype);
    	it_dsets->second->set_data_source(src);
    }

    // Set the maximum number of elements in each NDAttribute dataset
    size_t max_elements = this->conf.num_frames();
    HdfGroup::MapDatasets_t ndattr_dsets;
    this->layout.get_hdftree()->find_dsets(phdf_ndattribute, ndattr_dsets);
    for (it_dsets = ndattr_dsets.begin(); it_dsets != ndattr_dsets.end(); ++it_dsets)
    {
    	it_dsets->second->data_alloc_max_elements( max_elements );
    }

    // Configure the chunk cache for datasets
    DimensionDesc chunk_cache = this->conf.min_chunk_cache();
    this->rdcc_nslots = (size_t)this->conf.cache_num_slots(chunk_cache);
    this->rdcc_nbytes = chunk_cache.data_num_bytes();
    LOG4CXX_DEBUG(log, "Chunk cache: " << chunk_cache._str_() );
    LOG4CXX_DEBUG(log, "  - Number of slots in cache: " << this->rdcc_nslots );
    LOG4CXX_DEBUG(log, "  - Number of bytes in cache: " << this->rdcc_nbytes );
}

int NDArrayToHDF5::h5_open(const char *filename)
{
    LOG4CXX_TRACE(log, "h5_open()");
    int retcode = 0;
    this->conf.file_name(filename);

    herr_t hdfcode;
    hid_t file_access_plist = H5Pcreate(H5P_FILE_ACCESS);

#ifdef H5_HAVE_PARALLEL
    //LOG4CXX_DEBUG(log, "--------- HURRAH! WE ARE PARALLEL! ---------");
    int flag = 0;
    MPI_Initialized(&flag);
    if (flag != true) {
        LOG4CXX_WARN(log, "---- PARALLEL: but sadly MPI has *not* been initialized!");

    } else {
        LOG4CXX_DEBUG(log, "---- HURRAH! WE ARE PARALLEL: and MPI has been initialized!");

        // Configure the file access property list to use MPI communicator
        if (this->conf.is_io_mpiposix()) {
            LOG4CXX_DEBUG(log, " Configuring I/O: MPI + posix");
            hdfcode = H5Pset_fapl_mpiposix( file_access_plist, this->mpi_comm, 0 );
        } else {
            LOG4CXX_DEBUG(log, " Configuring I/O: MPI-I/O");
            hdfcode = H5Pset_fapl_mpio( file_access_plist, this->mpi_comm, this->mpi_info );
        }
         if (hdfcode < 0) {
            LOG4CXX_ERROR(log, "failed to set MPI communicator");
            H5Pclose(file_access_plist);
            return -1;
        }

    }
#endif

    // Disabling of the meta data cache is mentioned in various places as
    // a performance tweak. However for our data-model it does not appear to
    // have any significant impact.
/*
    LOG4CXX_INFO(log, "Disabling meta data cache" <<endl;
    H5AC_cache_config_t mdc_config;
    mdc_config.version = H5AC__CURR_CACHE_CONFIG_VERSION;
    hdfcode = H5Pget_mdc_config(file_access_plist, &mdc_config);
    if (hdfcode < 0) {
        LOG4CXX_WARN(log, " failed to get mdc_config for disabling metadata cache");
    } else {
        mdc_config.evictions_enabled = 0;
        mdc_config.incr_mode = H5C_incr__off;
        mdc_config.decr_mode = H5C_decr__off;
        mdc_config.flash_incr_mode = H5C_flash_incr__off;
        hdfcode = H5Pset_mdc_config(file_access_plist, &mdc_config);
        if (hdfcode < 0) {
            LOG4CXX_WARN(log, " failed to set mdc_config to disable metadata cache");
        }
    }
    */

    HSIZE_T alignment = this->conf.get_alignment();
    LOG4CXX_DEBUG(log, "Setting file alignment: " << alignment );
    hdfcode = H5Pset_alignment( file_access_plist, 64*1024, alignment);
    if (hdfcode < 0) {
        LOG4CXX_WARN(log, " failed to set alignement");
    }
    hid_t create_plist = H5Pcreate(H5P_FILE_CREATE);
    unsigned int istorek = this->conf.istorek();
    //long int istorek = 15000;
    if (istorek > 32) {
    	if (istorek > 75000) istorek = 75000;
        LOG4CXX_DEBUG(log, "Setting istorek: " << istorek);
        hdfcode = H5Pset_istore_k(create_plist, istorek);
        if (hdfcode < 0) {
            LOG4CXX_WARN(log, " failed to set i_store_k");
        }
    }
    H5Pget_istore_k(create_plist, &istorek);
    LOG4CXX_DEBUG(log, "istorek set to: " << istorek);

    // Ensure file is closed even if objects are still open.
    hdfcode = H5Pset_fclose_degree(file_access_plist, H5F_CLOSE_STRONG);
    if (hdfcode < 0) {
        LOG4CXX_WARN(log, " failed to file close degree to STRONG");
    }

    opentime.reset();
    this->h5file = H5Fcreate( filename, H5F_ACC_TRUNC, create_plist, file_access_plist);
    if (this->h5file == H5I_INVALID_HID) {
        LOG4CXX_ERROR(log, "unable to open/create file");
        H5Pclose(file_access_plist);
        H5Pclose(create_plist);
        return -1;
    }

    // Print out what kind of file I/O driver is enabled
    // The out-commented ones are unsupported in our environment.
    hid_t h5_driver = H5Pget_driver( file_access_plist );
    LOG4CXX_DEBUG(log, "File I/O driver: ");
    if (h5_driver == H5FD_SEC2) {
    	LOG4CXX_DEBUG(log, "    POSIX");
    } else if (h5_driver == H5FD_DIRECT) {
    	LOG4CXX_DEBUG(log, "    Direct");
    } else if (h5_driver == H5FD_LOG) {
    	LOG4CXX_DEBUG(log, "    POSIX with logging");
    //} else if (h5_driver == H5FD_WINDOWS)
    //    LOG4CXX_DEBUG(log, "    Windows");
    } else if (h5_driver == H5FD_STDIO) {
    	LOG4CXX_DEBUG(log, "    STDIO");
    } else if (h5_driver == H5FD_CORE) {
    	LOG4CXX_DEBUG(log, "    Memory");
    } else if (h5_driver == H5FD_FAMILY) {
    	LOG4CXX_DEBUG(log, "    Family");
    } else if (h5_driver == H5FD_MULTI) {
    	LOG4CXX_DEBUG(log, "    Multi");
    //} else if (h5_driver == H5FD_SPLIT)  {
    //	LOG4CXX_DEBUG(log, "    Split");
    } else if (h5_driver == H5FD_MPIO) {
    	LOG4CXX_DEBUG(log, "    Parallel MPIIO");
    } else if (h5_driver == H5FD_MPIPOSIX) {
    	LOG4CXX_DEBUG(log, "    Parallel POSIX");
    //else if (h5_driver == H5FD_STREAM)  {
    //	LOG4CXX_DEBUG(log, "    Stream");
    }

    H5Pclose(file_access_plist);
    H5Pclose(create_plist);
    opentime.stamp_now();
    LOG4CXX_INFO(log, opentime._str().c_str());

    // create the HDF5 file structure: groups and datasets
    retcode = this->create_file_layout();
    if (retcode < 0) {
    	LOG4CXX_ERROR(log, "Failed to create file layout");
    }

    return retcode;
}

int NDArrayToHDF5::h5_write(NDArray& ndarray)
{
    int retcode = 0;
    this->dt_write.dt_start();

    this->conf.next_frame(ndarray);
    LOG4CXX_TRACE(log, this->conf._str_());

    // debug
    NDAttribute *roi1;
    int32_t roi1_val=0;

    roi1 = ndarray.pAttributeList["h5_roi_origin_0"];
    if (roi1 != NULL) {
    	roi1->getValue(NDAttrInt32, &roi1_val, sizeof(int32_t));
    	LOG4CXX_TRACE(log, "origin offset: " << roi1_val );
    }


    HdfDataset *dset;
    HdfGroup::MapDatasets_t detector_dsets;
    this->layout.get_hdftree()->find_dsets(phdf_detector, detector_dsets);
    if (detector_dsets.size() <= 0)
    {
    	// If no dataset has been defined to receive the data, then we just return
    	LOG4CXX_WARN(log, "No detector dataset configured in file.");
    } else {
    	// TODO: here we just pick the first returned detector dataset. Later
    	//       we need to look up in the NDAttribute, the name of which dataset to use.
    	//       This will allow for writing different frames to different datasets (i.e.
    	//       separate raw data from flatfields, backgrounds etc.
    	dset = detector_dsets.begin()->second;

    	//int data = 77;
    	//this->_write_simple_frame(*dset, (void*)&data);
    	retcode = this->write_frame(dset, ndarray.pData);
    }

    this->cache_ndattributes(ndarray.pAttributeList);
    this->dt_write.dt_end();
    this->timestamp.stamp_now();
    return retcode;
}

int NDArrayToHDF5::h5_close()
{
    LOG4CXX_TRACE(log, "h5_close()");
    int retcode = 0;
    herr_t hdferr = 0;
    char fname[512] = "\0";

//    if (H5Iis_valid(this->h5file) == false) {
//    	LOG4CXX_ERROR(log, "h5file ID is not valid. File or library not open.");
//    	this->h5file = H5I_INVALID_HID;
//    	retcode = -1;
//    }

    if (this->h5file != H5I_INVALID_HID) {
    	this->write_ndattributes();
        LOG4CXX_INFO(log, "Writing profile data");
        this->store_profiling();
        H5Fget_name(this->h5file, fname, 512-1 );
        closetime.reset();
        LOG4CXX_DEBUG(log, "Remaining open objects: "
        	              << H5Fget_obj_count(this->h5file, H5F_OBJ_DATASET | H5F_OBJ_GROUP | H5F_OBJ_DATATYPE | H5F_OBJ_ATTR));

//        LOG4CXX_DEBUG(log, "Flushing file");
//        hdferr = H5Fflush(this->h5file, H5F_SCOPE_LOCAL);
//        LOG4CXX_DEBUG(log, "Done flushing: " << hdferr);

        LOG4CXX_INFO(log, "Closing file: " << fname);
        hdferr = H5Fclose(this->h5file);
        closetime.stamp_now();
        LOG4CXX_INFO(log, "File close time: " << closetime._str());
        this->h5file = H5I_INVALID_HID;
        if (hdferr < 0) {
        	LOG4CXX_ERROR(log, "Failed to close file: " << fname);
            retcode = -1;
        }
    }
    return retcode;
}

int NDArrayToHDF5::h5_reset()
{
    herr_t hdfreturn;
    LOG4CXX_INFO(log, "RESETTING! Closing down HDF5 library");
    hdfreturn = H5close();
    LOG4CXX_INFO(log, "Closed down HDF5 library: " << hdfreturn );
    LOG4CXX_INFO(log, "Reopening HDF5 library");
    hdfreturn = H5open();
    LOG4CXX_INFO(log, "Opened HDF5 library: " << hdfreturn );
    return hdfreturn;
}


void NDArrayToHDF5::cache_ndattributes( NDAttributeList& ndattr_list )
{
    // Run through all the NDAttributes and cache their values
    NDAttribute* ndattr = NULL;
    size_t ndattr_type_size=8;
    void * ptr_ndattr_data = calloc(8, sizeof(char));
    NDAttrDataType_t ndattr_type = NDAttrUndefined;
    HdfGroup::MapDatasets_t ndattr_dsets;
    HdfDataset *dset;
    string name;

    this->layout.get_hdftree()->find_dsets(phdf_ndattribute, ndattr_dsets);
    for (HdfGroup::MapDatasets_t::iterator it = ndattr_dsets.begin();
    	 it != ndattr_dsets.end();
    	 ++it)
    {
    	dset = it->second;
    	ndattr_type_size=8;
    	ndattr_type = NDAttrUndefined;
    	name = dset->get_name();

    	if (ndattr_list.count(name)) ndattr = ndattr_list[name];
    	else continue; // skip on to next if no NDAttribute of this name is in the list

    	ndattr->getValueInfo(&ndattr_type, &ndattr_type_size);
    	if (NDArrayToHDF5::from_ndattr_to_phdf_datatype(ndattr_type) !=
    			dset->data_source().get_datatype()) continue; // skip on to next if datatype does not match

    	if (ndattr_type_size != dset->data_source().datatype_size()) continue;

    	ndattr->getValue(ndattr_type, ptr_ndattr_data, ndattr_type_size);
    	dset->data_append_value(ptr_ndattr_data);
    }
    free(ptr_ndattr_data);
}

int NDArrayToHDF5::write_frame(HdfDataset * dset, void * ptr_data)
{
	int retval = 0;
	DatasetWriteParams_t params = {
			this->conf.get_dset_dims(),
			this->conf.get_roi_frame(),
			this->conf.get_dset_maxdims(),
			this->conf.get_offsets(),
			this->conf.get_chunk_dims(),
			this->conf.min_chunk_cache().data_num_bytes(),
			this->conf.is_dset_extendible(),
			NULL,
			ptr_data
	};

	// The frame size is 2 dimensions: X x Y pixels. However, HDF5 require
	// all writes to have the same dimensionality. So we extend it artificially
	// here to be like: 1 x X x Y.
	params.dims_frame_size.insert( params.dims_frame_size.begin(), 1);
	retval = this->write_dataset(dset, &params);
	return retval;
}


void NDArrayToHDF5::write_ndattributes()
{
	HdfGroup::MapDatasets_t ndattr_dsets;

	DatasetWriteParams_t params;
	params.dims_dset_size.push_back( this->mpi_size );
	params.dims_dset_size.push_back( this->conf.num_frames() );
	params.dims_frame_size = params.dims_dset_size;
	params.dims_frame_size[0] = 1;
	params.dims_offset.push_back(this->mpi_rank);
	params.dims_offset.push_back(0);
	params.extendible = true;

	this->layout.get_hdftree()->find_dsets(phdf_ndattribute, ndattr_dsets);
	for (HdfGroup::MapDatasets_t::iterator it = ndattr_dsets.begin();
			it != ndattr_dsets.end();
			++it)
	{
		HdfDataset *dset = it->second;
		params.dims_dset_size[1] = dset->data_num_elements();
		params.dims_frame_size[1] = dset->data_num_elements();
		params.pdata = dset->data();
		params.chunk_cache_bytes = dset->data_store_size();
		this->write_dataset(dset, &params);
		LOG4CXX_TRACE(log, " Data ptr: " << params.pdata
	           			<< " Dataset num_elements " << params.dims_dset_size[1]
	           			<< " size: " << params.chunk_cache_bytes);
		dset->data_stored();
	}
}

int NDArrayToHDF5::write_dataset(HdfDataset* dset, DatasetWriteParams_t *params)
{
	int retval = 0;
	string fullname("");
    hid_t h5_plist_dset_access = -1;
	hid_t h5_dataset;
    hid_t h5_file_dataspace = -1;
    hid_t h5_datatype = -1;
    hid_t h5_mem_dataspace = -1;
    hid_t h5_plist_xfer = -1;
    hid_t h5_err = -1;
    vec_ds_t dims_frame;
    hid_t dset_dtype;
    string smsg;

    fullname = dset->get_full_name();
    size_t num_dimensions = params->dims_dset_size.size();

    if (! H5Iis_valid(this->h5file)) {
    	LOG4CXX_ERROR(log, "file ID is not valid (file not open)");
    	return -1;
    }

    h5_plist_dset_access = H5Pcreate(H5P_DATASET_ACCESS);

    if (params->chunk_cache_bytes > 0) {
    	h5_err = H5Pset_chunk_cache(h5_plist_dset_access, this->rdcc_nslots, params->chunk_cache_bytes, 1.0);
    }

    h5_dataset = H5Dopen2(this->h5file, fullname.c_str(), h5_plist_dset_access);
    if (h5_dataset < 0) {
        LOG4CXX_ERROR(log, "unable to open h5_dataset");
        retval = -1;
        goto end_write_dataset;
    }

    if (params->extendible) {
        smsg = "Extending "  + fullname + " to: ";
        //print_arr( smsg.c_str(),
        //		WriteConfig::get_vec_ptr( params->dims_dset_size ),
        //		num_dimensions);
        LOG4CXX_TRACE(log, str_arr( smsg.c_str(),
        		WriteConfig::get_vec_ptr( params->dims_dset_size ),
        		num_dimensions));
        h5_err = H5Dset_extent( h5_dataset,
        		WriteConfig::get_vec_ptr( params->dims_dset_size ));
        if (h5_err < 0) {
            LOG4CXX_ERROR(log, "unable to extend dataset");
            retval = -1;
            goto end_write_dataset;
        }
    }

    h5_file_dataspace = H5Dget_space(h5_dataset);
    if (h5_file_dataspace < 0) {
        LOG4CXX_ERROR(log, "unable to get dataspace");
        retval = -1;
        goto end_write_dataset;
    }

    h5_datatype = H5Dget_type(h5_dataset);
    if (h5_datatype < 0) {
        LOG4CXX_ERROR(log, "unable to get datatype");
        retval = -1;
        goto end_write_dataset;
    }

    dset_dtype = NDArrayToHDF5::from_phdf_to_hid_datatype(dset->data_source().get_datatype());
    if (H5Tequal(h5_datatype, dset_dtype) <= 0)
    {
    	LOG4CXX_WARN(log,  "writing " << fullname << " datatypes dont appear to match (" \
    		 << h5_datatype << "!=" << dset_dtype << ")");
    }

    // Select the slab that we want to write to in the file
    dims_frame = params->dims_frame_size;
    dims_frame.insert(dims_frame.begin(), 1);
    smsg = "select_hyperslab "  + fullname + " start: ";
    //print_arr(smsg.c_str(), WriteConfig::get_vec_ptr( params->dims_offset ), params->dims_offset.size());
    LOG4CXX_TRACE(log, str_arr(smsg.c_str(), WriteConfig::get_vec_ptr( params->dims_offset ), params->dims_offset.size()));
    smsg = "select_hyperslab "  + fullname + " count: ";
    //print_arr(smsg.c_str(), WriteConfig::get_vec_ptr( params->dims_frame_size ), params->dims_frame_size.size());
    LOG4CXX_TRACE(log, str_arr(smsg.c_str(), WriteConfig::get_vec_ptr( params->dims_frame_size ), params->dims_frame_size.size()));
    h5_err = H5Sselect_hyperslab( h5_file_dataspace, H5S_SELECT_SET,
    							  WriteConfig::get_vec_ptr( params->dims_offset ), NULL,
                                  WriteConfig::get_vec_ptr( params->dims_frame_size ), NULL);
    if (h5_err < 0) {
        LOG4CXX_ERROR(log, "unable to select hyperslab");
        retval = -1;
        goto end_write_dataset;
    }

    // Memory data_space
    h5_mem_dataspace = H5Screate_simple(num_dimensions,
    		WriteConfig::get_vec_ptr( params->dims_frame_size ), NULL);
    if (h5_mem_dataspace < 0) {
        LOG4CXX_ERROR(log, "unable to create memory dataspace");
        retval = -1;
        goto end_write_dataset;
    }

    // Transfer property list to select collective or independent transfer
    h5_plist_xfer = H5P_DEFAULT;
#ifdef H5_HAVE_PARALLEL
    h5_plist_xfer = H5Pcreate(H5P_DATASET_XFER);
    if (h5_plist_xfer < 0) {
        LOG4CXX_ERROR(log, "unable to create transfer plist");
    } else if (not this->conf.is_io_mpiposix()){
        H5FD_mpio_xfer_t xfermode;
        xfermode = (this->conf.is_io_collective() ? H5FD_MPIO_COLLECTIVE : H5FD_MPIO_INDEPENDENT);
        h5_err = H5Pset_dxpl_mpio(h5_plist_xfer, xfermode);
        if (h5_err < 0)
        {
            LOG4CXX_ERROR(log, "unable to configure dataset I/O transfer type");
        }
    }
    LOG4CXX_TRACE(log, "Transfermode COLLECTIVE: " << this->conf.is_io_collective() );
#endif

    LOG4CXX_INFO(log, "Writing to: " << fullname << " [" << params->dims_offset[0] << "]");
    h5_err = H5Dwrite( h5_dataset, h5_datatype, h5_mem_dataspace,
                       h5_file_dataspace, h5_plist_xfer, params->pdata);
    if (h5_err < 0) {
        LOG4CXX_ERROR(log, "unable to write to dataset: " << fullname);
    	retval = -1;
    	goto end_write_dataset;
    }

    end_write_dataset:
	if (h5_dataset > 0) H5Dclose(h5_dataset);
	if (h5_file_dataspace > 0) H5Sclose(h5_file_dataspace);
	if (h5_mem_dataspace > 0) H5Sclose(h5_mem_dataspace);
	if (h5_plist_xfer > 0) H5Pclose(h5_plist_xfer);
    return retval;
}

WriteConfig NDArrayToHDF5::get_conf()
{
    return this->conf;
}

WriteConfig& NDArrayToHDF5::get_conf_ref()
{
    return this->conf;
}


/* Datatype translation functions. Just a simple lookup utility. */
PHDF_DataType_t NDArrayToHDF5::from_ndarr_to_phdf_datatype(NDDataType_t in) const
{
	PHDF_DataType_t out;
	switch(in)
	{
	case NDInt8:
		out = phdf_int8; break;
	case NDUInt8:
		out = phdf_uint8; break;
	case NDInt16:
		out = phdf_int16; break;
	case NDUInt16:
		out = phdf_uint16; break;
	case NDInt32:
		out = phdf_int32; break;
	case NDUInt32:
		out = phdf_uint32; break;
	case NDFloat32:
		out = phdf_float32; break;
	case NDFloat64:
		out = phdf_float64; break;
	default:
		out = phdf_int8;
		break;
	}
	return out;
}

PHDF_DataType_t NDArrayToHDF5::from_ndattr_to_phdf_datatype(NDAttrDataType_t in) const
{
	PHDF_DataType_t out;
	switch(in)
	{
	case NDAttrInt8:
		out = phdf_int8; break;
	case NDAttrUInt8:
		out = phdf_uint8; break;
	case NDAttrInt16:
		out = phdf_int16; break;
	case NDAttrUInt16:
		out = phdf_uint16; break;
	case NDAttrInt32:
		out = phdf_int32; break;
	case NDAttrUInt32:
		out = phdf_uint32; break;
	case NDAttrFloat32:
		out = phdf_float32; break;
	case NDAttrFloat64:
		out = phdf_float64; break;
	case NDAttrString:
		out = phdf_string; break;
	default:
		out = phdf_int8;
		break;
	}
	return out;
}


PHDF_DataType_t NDArrayToHDF5::from_hid_to_phdf_datatype(hid_t in) const
{
	PHDF_DataType_t out;
	if (in == H5T_NATIVE_INT8 )
		out = phdf_int8;
	else if (in == H5T_NATIVE_UINT8 )
		out = phdf_uint8;
	else if (in == H5T_NATIVE_INT16 )
		out = phdf_int16;
	else if (in == H5T_NATIVE_UINT16 )
		out = phdf_uint16;
	else if (in == H5T_NATIVE_INT32 )
		out = phdf_int32;
	else if (in == H5T_NATIVE_UINT32 )
		out = phdf_uint32;
	else if (in == H5T_NATIVE_FLOAT )
		out = phdf_float32;
	else if (in == H5T_NATIVE_DOUBLE )
		out = phdf_float64;
	else if (in == H5T_C_S1 )
		out = phdf_string;
	else {
		out = phdf_int8;
	}

	return out;
}

hid_t NDArrayToHDF5::from_ndarr_to_hid_datatype(NDDataType_t in) const
{
	hid_t out;
	switch(in)
	{
	case NDInt8:
		out = H5T_NATIVE_INT8; break;
	case NDUInt8:
		out = H5T_NATIVE_UINT8; break;
	case NDInt16:
		out = H5T_NATIVE_INT16; break;
	case NDUInt16:
		out = H5T_NATIVE_UINT16; break;
	case NDInt32:
		out = H5T_NATIVE_INT32; break;
	case NDUInt32:
		out = H5T_NATIVE_UINT32; break;
	case NDFloat32:
		out = H5T_NATIVE_FLOAT; break;
	case NDFloat64:
		out = H5T_NATIVE_DOUBLE; break;
	default:
		out = H5T_NATIVE_INT8;
		break;
	}
	return out;
}

hid_t NDArrayToHDF5::from_phdf_to_hid_datatype(PHDF_DataType_t in) const
{
	hid_t out;
	switch(in)
	{
	case phdf_int8:
		out = H5T_NATIVE_INT8; break;
	case phdf_uint8:
		out = H5T_NATIVE_UINT8; break;
	case phdf_int16:
		out = H5T_NATIVE_INT16; break;
	case phdf_uint16:
		out = H5T_NATIVE_UINT16; break;
	case phdf_int32:
		out = H5T_NATIVE_INT32; break;
	case phdf_uint32:
		out = H5T_NATIVE_UINT32; break;
	case phdf_float32:
		out = H5T_NATIVE_FLOAT; break;
	case phdf_float64:
		out = H5T_NATIVE_DOUBLE; break;
	case phdf_string:
		out = H5T_C_S1; break;
	default:
		out = H5T_NATIVE_INT8;
		break;
	}
	return out;
}



/*============== NDArrayToHDF5 private method implementations ================*/

/** Create the groups and datasets in the HDF5 file.
 */
int NDArrayToHDF5::create_file_layout()
{
    int retcode = 0;

    HdfRoot *root = this->layout.get_hdftree();
    LOG4CXX_DEBUG(log, "Root tree: " << root->_str_());
    retcode = this->create_tree(root, this->h5file);
    return retcode;
}

// utility method to help during debugging
void NDArrayToHDF5::print_arr (const char * msg, const hsize_t* sizes, size_t n)
{
	stringstream out(stringstream::out);
    out << msg << " [ ";
    for (unsigned int i=0; i<n; i++)
    {
        out << *(sizes+i) << ", ";
    }
    out << "]";
    LOG4CXX_DEBUG(log, out.str());
}
string NDArrayToHDF5::str_arr (const char * msg, const hsize_t* sizes, size_t n)
{
	stringstream out(stringstream::out);
    out << msg << " [ ";
    for (unsigned int i=0; i<n; i++)
    {
        out << " " << *(sizes+i) << ", ";
    }
    out << "]";
    return out.str();
}


/** Create the root group and recursively create all subgroups and datasets in the HDF5 file.
 *
 */
int NDArrayToHDF5::create_tree(HdfGroup* root, hid_t h5handle)
{
    int retcode = 0;
    if (root == NULL) return -1; // sanity check

    string name = root->get_name();
    //First make the current group inside the given hdf handle.
    hid_t new_group = H5Gcreate(h5handle, name.c_str(),
    							H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	if (new_group < 0)
	{
		LOG4CXX_ERROR(log, "failed to create group: " << name);
		return -1;
	}

    // Set some attributes on the group
	this->write_hdf_attributes( new_group,  root);

	// Create all the datasets in this group
	HdfGroup::MapDatasets_t::iterator it_dsets;
	HdfGroup::MapDatasets_t& datasets = root->get_datasets();
	for (it_dsets = datasets.begin(); it_dsets != datasets.end(); ++it_dsets)
	{
		hid_t new_dset = this->create_dataset(new_group, it_dsets->second);
		if (new_dset <= 0) continue; // failure to create the datasets so move on to next
		// Write the hdf attributes to the dataset
		this->write_hdf_attributes( new_dset,  it_dsets->second);
		H5Dclose(new_dset);
	}

    HdfGroup::MapGroups_t::const_iterator it;
    HdfGroup::MapGroups_t& groups = root->get_groups();
    for (it = groups.begin(); it != groups.end(); ++it)
    {
    	// recursively call this function to create all subgroups
    	retcode = this->create_tree( it->second, new_group );
    }
    // close the handle to the group that we've created in this instance
    // of the function. This is to ensure we're not leaving any hanging,
    // unused, and unreferenced handles around.
    H5Gclose(new_group);
    return retcode;
}

/** Run through the NDAttributeList from the detector and find or generate
 * corresponding entries in the HDFGroup tree.
 */
void NDArrayToHDF5::configure_ndattr_dsets(NDAttributeList& pAttributeList)
{
	PHDF_DataType_t datatype;
	HdfRoot * root = this->layout.get_hdftree();
	if (root == NULL) return;

	// first convert the NDAttributeList to a more practical std map.
	HdfGroup::MapNDAttrSrc_t map_ndattr;
	NDAttributeList::iterator attr_list_iter = pAttributeList.begin();
	for (attr_list_iter = pAttributeList.begin(); attr_list_iter != pAttributeList.end(); ++attr_list_iter)
	{
		datatype = NDArrayToHDF5::from_ndattr_to_phdf_datatype(attr_list_iter->second->getDataType());
		map_ndattr[attr_list_iter->second->getName()] = new HdfDataSource(phdf_ndattribute, datatype);
	}

	// Create a string set to contain the names of the NDAttributes which has
	// been allocated a space.
	set<string> ndattribute_names;
	// Run through the HDF tree and update any NDAttribute data sources
	root->merge_ndattributes(map_ndattr.begin(), map_ndattr.end(),
							 ndattribute_names);

}

hid_t NDArrayToHDF5::_create_simple_dset(hid_t group, HdfDataset *dset)
{
	hid_t dataset = -1;
    //hid_t dset_create_plist = -1;
    hid_t dataspace = -1;
    hsize_t dims[METADATA_NDIMS];
    //hsize_t maxdims[METADATA_NDIMS];
    dims[0] = 1;
    dims[1] = 1;

    LOG4CXX_DEBUG(log, "===== Creating meta data-set: " << dset->get_name() << " type: int");
    print_arr("--- dims:   " , dims, METADATA_NDIMS);

    dataspace = H5Screate_simple(METADATA_NDIMS, dims, NULL);

    /*
     * Create the dataset with default properties and close dataspace.
     */
    dataset = H5Dcreate2(group, dset->get_name().c_str(),
    		H5T_NATIVE_INT, dataspace,
			H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5Sclose(dataspace);
    return dataset;
}

int NDArrayToHDF5::_write_simple_frame(HdfDataset& dset, void *pdata)
{
	hid_t dataset = -1;
    //hid_t dataspace = -1;
    hid_t plist_id = -1;
    hid_t status = -1;

    LOG4CXX_TRACE(log, "Write dataset: " << dset.get_full_name() );
    hid_t dset_access_plist = H5Pcreate(H5P_DATASET_ACCESS);
    /*
     * Create property list for collective dataset write.
     */
    plist_id = H5Pcreate(H5P_DATASET_XFER);
    #ifdef H5_HAVE_PARALLEL
	H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
	#endif
	dataset = H5Dopen2(this->h5file,
			dset.get_full_name().c_str(),
			dset_access_plist);
    status = H5Dwrite(dataset,
    		H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
		    plist_id, pdata);
    H5Dclose(dataset);
    H5Pclose(plist_id);
    return status;
}

hid_t NDArrayToHDF5::create_dataset(hid_t group, HdfDataset *dset)
{
    int retcode = -1;
    if (dset == NULL) return -1; // sanity check

    if (dset->data_source().is_src_detector()) {
    	return this->create_dataset_detector(group, dset);
    	//return this->_create_simple_dset(group, dset);
    }

    if (dset->data_source().is_src_ndattribute()) {
    	return this->create_dataset_metadata(group, dset);
    	//return this->_create_dataset_detector(group, dset);
    	//return this->_create_simple_dset(group, dset);
    }

    // TODO : create datasets for profiling data...

    return retcode;
}



hid_t NDArrayToHDF5::create_dataset_metadata(hid_t group, HdfDataset* dset)
{
    hid_t dataset = -1;
    if (dset == NULL) return -1; // sanity check
    if (dset->data_source().is_src_detector() or
    	dset->data_source().is_src_constant()) return -1;
    int ndims = METADATA_NDIMS;
    int column = 0;

    hid_t dset_create_plist = -1;
    hid_t dataspace = -1;

    hsize_t dims[METADATA_NDIMS];
    hsize_t maxdims[METADATA_NDIMS];
    hsize_t chunkdims[METADATA_NDIMS];

    // If we're part of a larger MPI job (multiple processes)
    // then the metadata is in 2D tables.
    if (this->mpi_size > 0) {
    	column = this->mpi_rank;
    	dims[0] = this->mpi_size;
    	dims[1] = 1;
    	maxdims[0] = this->mpi_size;
    	maxdims[1] = H5S_UNLIMITED;
    	chunkdims[0] = 1;
    	chunkdims[1] = 1024; // TODO: figure out a sensible chunking scheme for meta-data
    } else { // else we're just doing a 1D dataset
    	column = 0;
    	dims[0] = 1;
    	maxdims[0] = H5S_UNLIMITED;
    	chunkdims[0] = 1024; // TODO: figure out a sensible chunking scheme for meta-data
    }

    // dataset create property list
    dset_create_plist = H5Pcreate(H5P_DATASET_CREATE);

    PHDF_DataType_t phdf_dtype = dset->data_source().get_datatype();
    hid_t datatype = NDArrayToHDF5::from_phdf_to_hid_datatype(phdf_dtype);

    /* Modify dataset creation properties, i.e. enable chunking  */
    //print_arr("Metadata chunking: ", chunkdims, ndims);
    H5Pset_chunk (dset_create_plist, ndims, chunkdims);

    dataspace = H5Screate_simple( ndims, dims, maxdims);
    if (dataspace < 0) {
        LOG4CXX_ERROR(log,  "failed to create dataspace for metadata dataset: " << dset->get_full_name());
        if (dset_create_plist > 0) H5Pclose(dset_create_plist);
        return -1;
    }

    const char * dsetname = dset->get_name().c_str();
    LOG4CXX_DEBUG(log, "Creating meta data-set: " << dsetname << " type: "<< datatype << ":" << phdf_dtype );
    print_arr("    metadata dims:   " , dims, METADATA_NDIMS);
    print_arr("    metadata chunks: " , chunkdims, METADATA_NDIMS);
    print_arr("    metadata max:    " , maxdims, METADATA_NDIMS);
    dataset = H5Dcreate2( group, dsetname,
                                datatype, dataspace,
                                H5P_DEFAULT, dset_create_plist, H5P_DEFAULT);
    if (dataset < 0) {
    	LOG4CXX_ERROR(log, "Failed to create dataset: " << dset->get_full_name() );
        if (dataspace > 0) H5Sclose(dataspace);
        if (dset_create_plist > 0) H5Pclose(dset_create_plist);
        return dataset;
    }

    if (dataspace > 0) H5Sclose(dataspace);
    if (dset_create_plist > 0) H5Pclose(dset_create_plist);

    return dataset;
}

/** Create a dataset in the HDF5 file with the details defined in the dset argument.
 * Return the hid_t handle to the new dataset on success; -1 on error.
 * Errors: fail to set chunk size or failure to create the dataset in the file.
 */
hid_t NDArrayToHDF5::create_dataset_detector(hid_t group, HdfDataset *dset)
{
    if (dset == NULL) return -1; // sanity check
    //if (!dset->data_source().is_src_detector()) return -1;
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
        LOG4CXX_ERROR(log,  "Failed to set chunksize ");
        return -1;
    }

    hid_t datatype = NDArrayToHDF5::from_phdf_to_hid_datatype(dset->data_source().get_datatype());

    // TODO: configure compression if required (although not for phdf5 as this would not work)

    // Setting the fill value
    // It appears that setting a fill value significantly decreases write performance! Ulrik 09.07.2013
/*
    char fillvalue[8] = {0,0,0,0,0,0,0,0};
    size_t size_fillvalue = 8 * sizeof(char);
    this->conf.get_fill_value((void*)fillvalue, &size_fillvalue);
    hdfcode = H5Pset_fill_value(dset_create_plist, datatype, (void*)fillvalue);
    if (hdfcode < 0) {
        LOG4CXX_WARN(log,  "Failed to set fill value");
    }
*/


    // Setting the dataset memory allocation time
    H5D_alloc_time_t alloc_time = (H5D_alloc_time_t)this->conf.get_alloc_time();
    LOG4CXX_DEBUG(log, "Storage allocation time: " << (int)alloc_time );
    hdfcode = H5Pset_alloc_time(dset_create_plist, alloc_time);
    if (hdfcode < 0) {
        LOG4CXX_WARN(log, "Failed to set dataset storage allocation time" );
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
        LOG4CXX_ERROR(log,  "failed to create dataspace for dataset: " << dset->get_full_name() );
        if (dset_create_plist > 0) H5Pclose(dset_create_plist);
        return -1;
    }

    const char * dsetname = dset->get_name().c_str();
    LOG4CXX_INFO(log, "===== Creating dataset: \"" << dsetname
    		<< "\" type: "<< datatype << ":" << dset->data_source().get_datatype() );
    dataset = H5Dcreate2( group, dsetname,
                                datatype, dataspace,
                                H5P_DEFAULT, dset_create_plist, H5P_DEFAULT);
    if (dataset < 0) {
        LOG4CXX_ERROR(log,  "Failed to create dataset: " << dset->get_full_name() );
        if (dataspace > 0) H5Sclose(dataspace);
        if (dset_create_plist > 0) H5Pclose(dset_create_plist);
        return dataset;
    }

    if (dataspace > 0) H5Sclose(dataspace);
    if (dset_create_plist > 0) H5Pclose(dset_create_plist);
    return dataset;
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
        LOG4CXX_ERROR(log,  "failed to create profiling dataspace" );
        return -1;
    }

    /* todo: fix hardcoded profiling dataset name */
    const char * dsetname = "profiling";
    LOG4CXX_INFO(log, "Creating dataset: " << dsetname );
    print_arr("Profiling dataset: ", profiling_dims, ndims);
    dataset = H5Dcreate2( this->h5file, dsetname,
                          H5T_NATIVE_DOUBLE, dataspace,
                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dataset < 0) {
        LOG4CXX_ERROR(log,  "Profiling: Failed to create dataset: " << dsetname );
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
            LOG4CXX_ERROR(log, "Profiling: unable to create memory dataspace");
            H5Sclose(file_dataspace);
            H5Dclose(dataset);
            return -1;
        }
        hdferr = H5Dwrite( dataset, H5T_NATIVE_DOUBLE, mem_dataspace,
                           file_dataspace, H5P_DEFAULT, &pf.front() );
        if (hdferr < 0) {
            LOG4CXX_ERROR(log, "Profiling: unable to write to dataset");
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

void NDArrayToHDF5::write_hdf_attributes( hid_t h5_handle, HdfElement* element)
{
	HdfElement::MapAttributes_t::iterator it_attr;
	PHDF_DataType_t attr_dtype = phdf_string;

	for (it_attr=element->get_attributes().begin();
			it_attr != element->get_attributes().end();
			++it_attr)
	{
		HdfAttribute &attr = it_attr->second; // Take a reference - i.e. *not* a copy!
		attr_dtype = attr.source.get_datatype();
		switch ( attr_dtype )
		// if string
		{
		case phdf_string:
			this->write_h5attr_str(h5_handle,
					attr.get_name(),
					attr.source.get_src_def());
			break;
		case phdf_float64:
			break;
		case phdf_int32:
			break;
		default:
			break;
		}
	}
}

void NDArrayToHDF5::write_h5attr_str(hid_t element,
		const string &attr_name,
		const string &str_attr_value) const
{
	herr_t hdfstatus = -1;
	hid_t hdfdatatype = -1;
	hid_t hdfattr = -1;
	hid_t hdfattrdataspace = -1;

	hdfattrdataspace = H5Screate(H5S_SCALAR);
	hdfdatatype      = H5Tcopy(H5T_C_S1);
	hdfstatus        = H5Tset_size(hdfdatatype, str_attr_value.size());
	hdfstatus        = H5Tset_strpad(hdfdatatype, H5T_STR_NULLTERM);
	hdfattr          = H5Acreate2(element, attr_name.c_str(),
			hdfdatatype, hdfattrdataspace,
			H5P_DEFAULT, H5P_DEFAULT);
	if (hdfattr < 0) {
		LOG4CXX_ERROR(log,  "unable to create attribute: " << attr_name );
		H5Sclose(hdfattrdataspace);
		return;
	}

	hdfstatus = H5Awrite(hdfattr, hdfdatatype, str_attr_value.c_str());
	if (hdfstatus < 0) {
		LOG4CXX_ERROR(log,  "unable to write attribute: " << attr_name );
		H5Aclose (hdfattr);
		H5Sclose(hdfattrdataspace);
	}
	H5Aclose (hdfattr);
	H5Sclose(hdfattrdataspace);
	return;
}

void NDArrayToHDF5::write_h5attr_number(hid_t element, const string& attr_name,
		DimensionDesc dims,
		PHDF_DataType_t datatype, void * data) const
{
	hid_t h5_attr;
	hid_t h5_err;
	hid_t h5_dspace;
	const hsize_t *h5_dims = dims.dim_sizes();
	hid_t h5_dtype = NDArrayToHDF5::from_phdf_to_hid_datatype(datatype);

	h5_dspace = H5Screate_simple(dims.num_dimensions(), h5_dims, NULL);

	// Create a dataset attribute.
	h5_attr = H5Acreate2 (element, attr_name.c_str(),
			h5_dtype, h5_dspace,
			H5P_DEFAULT, H5P_DEFAULT);
	if (h5_attr < 0) {
		LOG4CXX_ERROR(log,  "unable to create attribute: " << attr_name );
		H5Sclose(h5_dspace);
		return;
	}

	// Write the attribute data.
	h5_err = H5Awrite(h5_attr, h5_dtype, data);
	if (h5_err < 0) {
		LOG4CXX_ERROR(log,  "unable to write attribute: " << attr_name );
		H5Aclose(h5_attr);
		H5Sclose(h5_dspace);
		return;
	}

	// Close the attribute.
	h5_err = H5Aclose(h5_attr);

	// Close the dataspace.
	h5_err = H5Sclose(h5_dspace);
}

