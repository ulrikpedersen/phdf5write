/*
 * writeconfig.cpp
 *
 *  Created on: 11 Jan 2012
 *      Author: up45
 */
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
//#include <stdio.h>
//#include <stdlib.h>

#include <NDArray.h>
#include "dimension.h"
#include "writeconfig.h"
#include "phdf5util.h"

namespace phdf5 {

#define FILL_VALUE_SIZE 8

const std::string WriteConfig::ATTR_CHUNK_SIZE  = "h5_chunk_size_";
const std::string WriteConfig::ATTR_ROI_ORIGIN  = "h5_roi_origin_";
const std::string WriteConfig::ATTR_DSET_SIZE   = "h5_dset_size_";
const std::string WriteConfig::ATTR_FILL_VAL    = "h5_fill_val";
const std::string WriteConfig::ATTR_COMPRESSION = "h5_compression";

/*========================================================================
 *
 * Public interface functions
 *
 * ======================================================================= */

/** Default constructor to set sensible default values
 * for all options.
 */
WriteConfig::WriteConfig()
:alignment(H5_DEFAULT_ALIGN), alloc_time(0)
{
    this->_default_init();
}


WriteConfig::WriteConfig(NDArray& ndarray, int mpi_rank, int mpi_proc)
:alignment(H5_DEFAULT_ALIGN), alloc_time(0)
{
    this->_default_init();
    this->proc_rank_size(mpi_rank, mpi_proc);
    this->parse_ndarray_attributes(ndarray);
}

WriteConfig::WriteConfig(const WriteConfig& src)
:alignment(H5_DEFAULT_ALIGN), alloc_time(0)
{
    this->_default_init();
    this->_copy(src);
}

/*
WriteConfig::WriteConfig(string& filename)
:alignment(H5_DEFAULT_ALIGN)
{
    this->_default_init();
}
*/

WriteConfig::~WriteConfig()
{
    free( this->ptr_fill_value );
}

WriteConfig& WriteConfig::operator =(const WriteConfig& src)
{
    // Check for self-assignment
    if (this == &src)
        return *this;
    // perform the copy
    this->_copy(src);
    return *this;
}


std::string WriteConfig::file_name()
{
    return this->str_file_name;
}

void WriteConfig::proc_rank_size(int rank, int size)
{
    // Limit the proc_rank value to a maximum of size-1
    // If the rank value is less than size then use rank
    if (rank < size) this->proc_rank = rank;
    else this->proc_rank = size-1;

    // Limit the proc_size value to a minimum of 1
    if (size < 1) this->proc_size = 1;
    else this->proc_size = size;
    //LOG4CXX_DEBUG(log,  "WriteConfig: setting rank=" << this->proc_rank << " proc_size=" << this->proc_size );
}

int WriteConfig::num_extra_dims()
{
    int num_img_dims = this->dim_roi_frame.num_dimensions();
    int num_dset_dims = this->dim_full_dataset.num_dimensions();
    int extra_dims = num_dset_dims - num_img_dims;
    return extra_dims>0 ? extra_dims : 0;
}

long int WriteConfig::num_frames()
{
	long int result = 1;
	for (int dim = 0;
		 dim < this->dim_full_dataset.num_dimensions() - this->dim_roi_frame.num_dimensions();
		 dim++)
	{
		result *= this->dim_full_dataset.dim_size_vec().at(dim);
	}
	return result;
}

void WriteConfig::inc_position(NDArray& ndarray)
{
    NDAttributeList * list = ndarray.pAttributeList;
    std::string attr_name;
    vec_ds_t attr_origins;
    vec_ds_t::iterator it_origen;
    int idim = 0;
    int ret = 0;
    vec_ds_t full_dset_dims = this->dim_full_dataset.dim_size_vec();
    vec_ds_t roi_frame_dims = this->dim_roi_frame.dim_size_vec();
    int num_img_dims = this->dim_roi_frame.num_dimensions();

    attr_name = WriteConfig::ATTR_ROI_ORIGIN;
    ret = this->get_attr_array(attr_name, list, attr_origins);
    if (ret == this->dim_full_dataset.num_dimensions()) {
        // Origins/Offsets from the NDArray attribute if they are available for every dimension.
        this->origin = attr_origins;
        //LOG4CXX_DEBUG(log,  "GOT ROI offset from attributes!" );
    } else {
        LOG4CXX_DEBUG(log,  "ATTRIBUTES NOT LISTING ROI" );
        // if origin not available as NDAttr for every dimension then
        // we increment manually based on some assumptions (see comments below)

        // If this is the first frame to write then we need to create the origin
        // vector and initialise it.
        if (this->origin.size() <= 0) {
            // Get the position/origin of the ROI within the full detector frame or
            // the full dataset.

            // Most origins are set to 0 to begin with
            this->origin = vec_ds_t(full_dset_dims.size(), 0);
            // .. except if we are running multiple processes
            // in which case we assume an even split in the ROI frame slowest changing
            // dimension (for instance a 2D image is split in horizontal stripes)
            size_t split_dim = num_img_dims - 1;
            if (this->proc_rank > 0)
            	this->origin[split_dim] = roi_frame_dims[0] * this->proc_rank;
        } else {

            idim = 0;

            // Loop through extra dimensions to increment one frame in origin/offset
            // Iterator for the origen vector starts at the first extra dimension.
            // (i.e. after the image width, height dimensions...)
            int num_extra_dims = this->num_extra_dims();
            for (it_origen = this->origin.begin();
                    it_origen != this->origin.begin()+num_extra_dims;
                    ++it_origen, idim++)
            {
                (*it_origen)++;

                // check if the end of this dimension has been reached
                if (*it_origen == full_dset_dims.at(idim)) {
                    // Normally we will reset the origin here and next iteration will
                    // increment the next dim -except if this is the last dimension, in
                    // which case we dont reset because we could end up overwriting data.
                    LOG4CXX_DEBUG(log,  "    --set full dimsize: " << idim << " " << this->dim_full_dataset );
                    if (it_origen == this->origin.begin() + num_extra_dims) {
                        this->dim_full_dataset.set_dimension_size(idim, full_dset_dims.at(idim) + 1);
                        LOG4CXX_DEBUG(log,  " set full dimsize: " << idim << " " << this->dim_full_dataset );
                        break;
                    }
                    else *it_origen=0;
                } else {
                    // else we are done: just incremented one dim and finish
                    break;
                }
            }
        }
    }

    // Loop through extra dimensions to extend the size of the active dataset
    // to fit the new frame. Note that the active dataset must never be made smaller
    // (just in case frames arrive out of expected order within the MPI job)
    idim = 0;
    for (it_origen = this->origin.begin();
            it_origen != this->origin.end()-num_img_dims;
            ++it_origen, idim++)
    {
        dimsize_t dimsize = this->dim_active_dataset.dim_size_vec().at(idim);
        if (dimsize < *it_origen + 1) {
            this->dim_active_dataset.set_dimension_size(idim, *it_origen + 1);
        }
    }
}

/** Next frame to process. Increment the state of origin.
 *
 * TODO: Full implementation of the next_frame function. Currently it only calls inc_position.
 */
int WriteConfig::next_frame(NDArray& ndarray)
{
    // First check whether this instance has been configured with or without
    // an initial NDArray. If not, run the NDArray attribute parser to initialise.
    if (this->dim_roi_frame.num_dimensions() == 0) {
        this->parse_ndarray_attributes(ndarray);
    }

    this->inc_position(ndarray);

    // TODO: what can we do about dropped/lost frames here?
    return 0;
}

void WriteConfig::get_fill_value(void *fill_value, size_t *max_size_bytes)
{
    size_t num_bytes = this->dim_roi_frame.element_size;
    if (num_bytes > *max_size_bytes) num_bytes = *max_size_bytes;
    memcpy(fill_value, this->ptr_fill_value, num_bytes);
    *max_size_bytes = num_bytes;
}

/** Return the istore_k based on the chunking and full dataset size
 * According to HDF5 forum an appropriate value is to use a value which is
 * half the number of chunks in the full dataset.
 */
unsigned int WriteConfig::istorek()
{
    unsigned int istore_k = 0;
    unsigned int num_chunks_dset = 0;
    num_chunks_dset = this->dim_chunk.num_fits( this->dim_full_dataset, true);
    LOG4CXX_DEBUG(log,  "istorek num_chunks_dset: " << num_chunks_dset );
    // if invalid (i.e. the dataset size is not known or too small) we return error
    if (num_chunks_dset <= 0) return 0;

    istore_k = num_chunks_dset >> 1;
    return istore_k;
}

DimensionDesc WriteConfig::min_chunk_cache()
{
    DimensionDesc min_cache_block = this->dim_roi_frame;

    for (int i=this->dim_roi_frame.num_dimensions();
            i < this->dim_chunk.num_dimensions(); i++) {
        min_cache_block += 1;
    }
    if (min_cache_block.grow_by_block(this->dim_chunk) < 0)
        return this->dim_roi_frame;

    return min_cache_block;
}

HSIZE_T WriteConfig::get_alignment()
{
    return this->alignment;
}

/** Calculate the number of 'slots' in the cache.
 * According to HDF5 documentation this is advised to be a prime number of
 * about 10 to 100 times the number of chunks that fit in the cache.
 * Returns a prime number. If the chunk dimensions does not fit in the
 * cache_block a 0 is returned.
 */
unsigned long int WriteConfig::cache_num_slots( DimensionDesc& cache_block )
{
    LOG4CXX_DEBUG(log,  "num_fits in cache block" );

    int num_chunks = this->dim_chunk.num_fits(cache_block, true);
    if (num_chunks <= 0) return 0; // a fault: the cache_block is smaller than the chunk dimensions

    unsigned long int start_range =  10*num_chunks;
    unsigned long int end_range   = 100*num_chunks;
    LOG4CXX_DEBUG(log,  "start_range: " << start_range );
    std::vector<unsigned int long> range (end_range-start_range, 0);

    // Range of numbers from 10 to 100 times the number of chunks in the cache block
    for (unsigned long int i=start_range; i<end_range; i++) range[i-start_range] = i;

    // find the first prime in the range
    std::vector<unsigned int long>::iterator it_first_prime;
    it_first_prime = find_if(range.begin(), range.end(), is_prime);

    if (*it_first_prime <= 0) return 10*num_chunks;

    return *it_first_prime;
}

std::string WriteConfig::_str_()
{
    std::stringstream out(std::stringstream::out);
    out << "<WriteConfig:";
    //out << " filename=  " << this->file_name();
    out << " rank=" << this->proc_rank << " nproc=" << this->proc_size;
    out << "\n\t\t\tROI:    " << this->dim_roi_frame;
    out << "\n\t\t\tchunk:  " << this->dim_chunk;
    out << "\n\t\t\tdset:   " << this->dim_full_dataset;
    out << "\n\t\t\tactive: " << this->dim_active_dataset;
    out << "\n\t\t\toffset: " << DimensionDesc(this->origin, this->dim_roi_frame.element_size)._str_();
    out << "\n\t\t\tposix=" << this->mpiposix << " collective=" << this->iocollective << " extendible=" << this->extendible;
    out << "\n\t\t /WriteConfig>";
    return out.str();
}

/*========================================================================
 *
 * Private method implementations
 *
 * ======================================================================= */
void WriteConfig::_default_init()
{
	log = log4cxx::Logger::getLogger("WriteConfig");
    this->ptr_fill_value = (void*)calloc(FILL_VALUE_SIZE, sizeof(char));
    this->proc_rank = 0;
    this->proc_size = 1;
    this->iocollective = false;
    this->mpiposix = false;
    this->extendible = true;
}

void WriteConfig::_copy(const WriteConfig& src)
{
    memcpy(this->ptr_fill_value, src.ptr_fill_value, FILL_VALUE_SIZE);

    this->alignment = src.alignment;
    this->dim_chunk = src.dim_chunk;
    this->dim_roi_frame = src.dim_roi_frame;
    this->dim_full_dataset = src.dim_full_dataset;
    this->dim_active_dataset = src.dim_active_dataset;
    this->origin = src.origin;
    this->str_file_name = src.str_file_name;
    this->proc_rank = src.proc_rank;
    this->proc_size = src.proc_size;
    this->iocollective = src.iocollective;
    this->mpiposix = src.mpiposix;
    this->extendible = src.extendible;
}

DimensionDesc WriteConfig::get_detector_dims()
{
    vec_ds_t tmpdims;
    // Get the individual detector frame size from a combination of the ndarray (ROI)
    // and the full dataset size. The ROI will tell how many dimensions the detector produce
    // and the actual size can be found from the last dimensions of the full dataset size.
    int ndims = this->dim_roi_frame.num_dimensions();
    vec_ds_t vec_dset = this->dim_full_dataset.dim_size_vec();
    int num_extra_dims = this->num_extra_dims();
    tmpdims.assign(vec_dset.begin()+num_extra_dims, vec_dset.end());
    return DimensionDesc(tmpdims, this->dim_roi_frame.element_size);
}

/** Return a vector of integers. The first dimensions will be the frame size and
 * the remaining will be set to -1 to indicate extendible dimensions.
 */
vec_ds_t WriteConfig::get_dset_maxdims()
{
    vec_ds_t maxdims = this->dim_full_dataset.dim_size_vec();
    int num_extra_dims = this->num_extra_dims();

    if (this->extendible) {
        for (int i = 0; i<num_extra_dims; i++)
        {
            maxdims[i] = -1;
        }
    }
    //maxdims.insert( maxdims.end(), num_extra_dims, -1);
    //maxdims.assign( num_extra_dims, -1);
    return maxdims;
}


int WriteConfig::get_attr_fill_val(NDAttributeList *ptr_attr_list)
{
    NDAttribute *ptr_ndattr = NULL;
    NDAttrDataType_t attr_data_t;
    size_t attr_size = 0;

    // By default we set the fillvalue to 0
    memset(this->ptr_fill_value, 0, FILL_VALUE_SIZE);

    ptr_ndattr = ptr_attr_list->find(WriteConfig::ATTR_FILL_VAL.c_str());
    // Check whether the attribute exist in the list
    if (ptr_ndattr == NULL) return -1;

    // Get the datatype and size of the attribute
    ptr_ndattr->getValueInfo(&attr_data_t, &attr_size);
    if (attr_data_t > FILL_VALUE_SIZE) return -1;
    ptr_ndattr->getValue(attr_data_t, this->ptr_fill_value, attr_size);
    return 0;
}

int WriteConfig::get_alloc_time()
{
	return this->alloc_time;
}

/**
 * Parse a a series of attributes formatted: "h5_nnnn_%d"
 */
int WriteConfig::get_attr_array(std::string& attr_name, NDAttributeList *ptr_attr_list,
                                vec_ds_t& dst)
{
    unsigned int i = 0;
    int retcode = 0;
    int attr_value = 0;
    while (retcode >= 0)
    {
        std::stringstream ss_attr(std::stringstream::out);
        ss_attr << attr_name << i;
        retcode = this->get_attr_value(ss_attr.str(), ptr_attr_list, &attr_value);
        //LOG4CXX_DEBUG(log,  "get_attr_values (" << ss_attr.str() << ")return: " << attr_value );
        if (retcode >= 0) dst.push_back((unsigned long)attr_value);
        else break;
        i++;
    }
    return dst.size();
}

/** Parse the attributes of an NDArray and use the information
 * to populate the private data structures.
 *
 */
void WriteConfig::parse_ndarray_attributes(NDArray& ndarray)
{
    NDAttributeList * list = ndarray.pAttributeList;
    std::string attr_name;
    vec_ds_t tmpdims;
    int ret=0;

    // Get the dimensions of the ndarray.
    // By default we set the chunk size and full frame size to be
    // equal to the current frame. Later we override chunk and full size
    // if this information is available in the ndarray attributes.
    this->dim_roi_frame = DimensionDesc(ndarray);
    this->dim_chunk = this->dim_roi_frame;
    this->dim_full_dataset = this->dim_roi_frame;
    this->dim_chunk += 1;
    this->dim_full_dataset += 1;
    // In case we are running on multiple processes the received frame is only
    // an ROI subset of a larger frame. The assumption (if NDAttributes don't hint
    // otherwise) is that the split is done in the slowest changing dimension.
    // So for example a 2D image is split in horizontal stripes.
    int split_dim = this->dim_full_dataset.num_dimensions() -2;
    dimsize_t split_dim_full_size = 0;
    if (split_dim >= 0) {
        split_dim_full_size = *(this->dim_roi_frame.dim_sizes()+0) * this->proc_size;
        this->dim_full_dataset.set_dimension_size( split_dim, split_dim_full_size);
    }

    // Get the chunking size from the NDArray attributes
    attr_name = WriteConfig::ATTR_CHUNK_SIZE;
    tmpdims.clear();
    ret = this->get_attr_array(attr_name, list, tmpdims);
    if (ret > 0) {
        this->dim_chunk = DimensionDesc(tmpdims, this->dim_roi_frame.element_size);

    }
    //LOG4CXX_DEBUG(log,  "Chunk size: " << this->dim_chunk );

    // Get the full dataset size from the NDArray attributes
    attr_name = WriteConfig::ATTR_DSET_SIZE;
    tmpdims.clear();
    ret = this->get_attr_array(attr_name, list, tmpdims);
    if (ret > 0) {
        this->dim_full_dataset = DimensionDesc(tmpdims, this->dim_roi_frame.element_size);
    }
    //LOG4CXX_DEBUG(log,  "full dataset: " << this->dim_full_dataset );

    // Configure the current active dataset with the frame dimensions and the
    // additional dimensions all set to 1
    this->dim_active_dataset = this->get_detector_dims();
    vec_ds_t vec_act_dset = this->dim_active_dataset.dim_size_vec();
    int nextradims = this->dim_full_dataset.num_dimensions() - this->dim_roi_frame.num_dimensions();
    if (nextradims > 0) {
        vec_act_dset.insert(vec_act_dset.begin(), nextradims, 1);
        this->dim_active_dataset = DimensionDesc(vec_act_dset, this->dim_roi_frame.element_size);
    }
    //LOG4CXX_DEBUG(log,  this->dim_active_dataset );

    /* Collect the fill value from the ndarray attributes */
    ret = this->get_attr_fill_val(list);
    //LOG4CXX_DEBUG(log,  "get_attr_fill_val returns: " << ret << " value: " << *((unsigned long*)this->ptr_fill_value) );

    //LOG4CXX_DEBUG(log,  *this );
}

int WriteConfig::get_attr_value(const std::string& attr_name, NDAttributeList *ptr_attr_list, int *attr_value)
{
    long int retval = 0;
    int retcode = 0;
    NDAttribute *ptr_ndattr;
    //LOG4CXX_DEBUG(log,  "getting attribute: " << attr_name );
    ptr_ndattr = ptr_attr_list->find(attr_name.c_str());
    if (ptr_ndattr == NULL) return -1;
    if (ptr_ndattr->dataType ==  NDAttrString ) return -1;

    retcode = ptr_ndattr->getValue(NDAttrUInt32, (void*)attr_value, sizeof(long int));
    if (retcode == ND_ERROR) {
        retcode = ptr_ndattr->getValue(NDAttrInt32, (void*)attr_value, sizeof(long int));
    }
    if (retcode == ND_ERROR) {*attr_value=0; return -1;}
    return retval;
}

HSIZE_T * WriteConfig::get_vec_ptr(vec_ds_t& vec)
{
    return (HSIZE_T*)&vec.front();
}


bool WriteConfig::delay_dim_config()
{
    if (this->dim_roi_frame.num_dimensions() <= 0) return false;
    else return true;
}

} // phdf5

