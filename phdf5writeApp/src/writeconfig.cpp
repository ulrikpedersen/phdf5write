/*
 * writeconfig.cpp
 *
 *  Created on: 11 Jan 2012
 *      Author: up45
 */
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

#include <NDArray.h>
#include "dimension.h"
#include "writeconfig.hpp"

#define FILL_VALUE_SIZE 8

/*========================================================================
 *
 * Public interface functions
 *
 * ======================================================================= */

/** Default constructor to set sensible default values
 * for all options.
 */
WriteConfig::WriteConfig()
:alignment(H5_DEFAULT_ALIGN)
{
    this->_default_init();
}


WriteConfig::WriteConfig(NDArray& ndarray)
:alignment(H5_DEFAULT_ALIGN)
{
    this->_default_init();
    this->parse_ndarray_attributes(ndarray);
}

WriteConfig::WriteConfig(const WriteConfig& src)
:alignment(H5_DEFAULT_ALIGN)
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


string WriteConfig::file_name()
{
    return this->str_file_name;
}

void WriteConfig::inc_position(NDArray& ndarray)
{
    NDAttributeList * list = ndarray.pAttributeList;
    string attr_name;
    vec_ds_t attr_origins;
    vec_ds_t::iterator it_origen;
    int idim = 0;
    int ret = 0;

    attr_name = ATTR_ROI_ORIGIN;
    ret = this->get_attr_array(attr_name, list, attr_origins);
    if (ret == this->dim_full_dataset.num_dimensions()) {
        // Origins/Offsets from the NDArray attribute if they are available for every dimension.
        this->origin = attr_origins;
    } else {
        // if origin not available as NDAttr for every dimension then
        // we increment manually.

        // If this is the first frame to write then we need to create the origin
        // vector and initialise it to zero
        if (this->origin.size() <= 0) {
            // Get the position/origin of the ROI within the full detector frame or
            // the full dataset.
            // By default all origins are set to 0
            this->origin = vec_ds_t(this->dim_full_dataset.num_dimensions(), 0);
        } else {
            // Iterator for the origen vector, which starts at the first extra dimension.
            // (i.e. after the image width, height dimensions...)
            vec_ds_t full_dset_dims = this->dim_full_dataset.dim_size_vec();
            idim = this->dim_roi_frame.num_dimensions();

            // Loop through extra dimensions to increment one frame
            // in origin/offset and the active dataset
            for (it_origen = this->origin.begin()+idim;
                    it_origen != this->origin.end();
                    ++it_origen, idim++)
            {
                (*it_origen)++;

                // check if the end of this dimension has been reached
                if (*it_origen == full_dset_dims.at(idim)) {
                    // reset if it has and next iteration will increment next dim
                    *it_origen=0;
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
    idim = this->dim_roi_frame.num_dimensions();
    for (it_origen = this->origin.begin()+idim;
         it_origen != this->origin.end();
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
    size_t num_bytes = FILL_VALUE_SIZE;
    if (num_bytes > *max_size_bytes) num_bytes = *max_size_bytes;
    memcpy(fill_value, this->ptr_fill_value, num_bytes);
    *max_size_bytes = num_bytes;
}

/** Return the istore_k based on the chunking and full dataset size
 * According to HDF5 forum an appropriate value is to use a value which is
 * half the number of chunks in the full dataset.
 */
long int WriteConfig::istorek()
{
    int istore_k = 0;
    int num_chunks_dset = 0;
    num_chunks_dset = this->dim_chunk.num_fits( this->dim_full_dataset, true);
    cout << "istorek num_chunks_dset: " << num_chunks_dset << endl;
    // if invalid (i.e. the dataset size is not known or too small) we return error
    if (num_chunks_dset <= 0) return -1;

    istore_k = num_chunks_dset << 1;
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


/** Calculate the number of 'slots' in the cache.
 * According to HDF5 documentation this is advised to be a prime number of
 * about 10 to 100 times the number of chunks that fit in the cache.
 * Returns a prime number. If the chunk dimensions does not fit in the
 * cache_block a 0 is returned.
 */
unsigned long int WriteConfig::cache_num_slots( DimensionDesc& cache_block )
{
    cout << "num_fits in cache block" << endl;

    int num_chunks = this->dim_chunk.num_fits(cache_block, true);
    if (num_chunks <= 0) return 0; // a fault: the cache_block is smaller than the chunk dimensions

    unsigned long int start_range =  10*num_chunks;
    unsigned long int end_range   = 100*num_chunks;
    cout << "start_range: " << start_range << endl;
    vector<unsigned int long> range (end_range-start_range, 0);

    // Range of numbers from 10 to 100 times the number of chunks in the cache block
    for (unsigned long int i=start_range; i<end_range; i++) range[i-start_range] = i;

    // find the first prime in the range
    vector<unsigned int long>::iterator it_first_prime;
    it_first_prime = find_if(range.begin(), range.end(), is_prime);

    if (*it_first_prime <= 0) return 10*num_chunks;

    return *it_first_prime;
}

string WriteConfig::_str_()
{
    stringstream out(stringstream::out);
    out << "<WriteConfig:";
    out << " filename=  " << this->file_name();
    out << "\n\tROI:    " << this->dim_roi_frame;
    out << "\n\tchunk:  " << this->dim_chunk;
    out << "\n\tdset:   " << this->dim_full_dataset;
    out << "\n\tactive: " << this->dim_active_dataset;
    out << "\n\toffset: " << DimensionDesc(this->origin, this->dim_roi_frame.element_size)._str_();
    out << "\n /WriteConfig>";
    return out.str();
}

/*========================================================================
 *
 * Private method implementations
 *
 * ======================================================================= */
void WriteConfig::_default_init()
{
    this->ptr_fill_value = (void*)calloc(FILL_VALUE_SIZE, sizeof(char));
}

void WriteConfig::_copy(const WriteConfig& src)
{
    memcpy(this->ptr_fill_value, src.ptr_fill_value, FILL_VALUE_SIZE);

    this->dim_chunk = src.dim_chunk;
    this->dim_roi_frame = src.dim_roi_frame;
    this->dim_full_dataset = src.dim_full_dataset;
    this->dim_active_dataset = src.dim_active_dataset;
    this->origin = src.origin;
    this->str_file_name = src.str_file_name;
}

DimensionDesc WriteConfig::get_detector_dims()
{
    vec_ds_t tmpdims;
    // Get the individual detector frame size from a combination of the ndarray (ROI)
    // and the full dataset size. The ROI will tell how many dimensions the detector produce
    // and the actual size can be found from the first dimensions of the full dataset size.
    int ndims = this->dim_roi_frame.num_dimensions();
    vec_ds_t vec_dset = this->dim_full_dataset.dim_size_vec();
    tmpdims.assign(vec_dset.begin(), vec_dset.begin()+ndims);
    return DimensionDesc(tmpdims, this->dim_roi_frame.element_size);
}

/** Return a vector of integers. The first dimensions will be the frame size and
 * the remaining will be set to -1 to indicate extendible dimensions.
 */
vec_ds_t WriteConfig::get_dset_maxdims()
{
    vec_ds_t detector_dims = this->get_detector_dims().dim_size_vec();
    vec_ds_t maxdims( detector_dims.begin(), detector_dims.end() );
    int num_extra_dims = this->dim_full_dataset.num_dimensions() - maxdims.size();

    maxdims.insert( maxdims.end(), num_extra_dims, -1);
    return maxdims;
}

int WriteConfig::get_attr_fill_val(NDAttributeList *ptr_attr_list)
{
    NDAttribute *ptr_ndattr = NULL;
    NDAttrDataType_t attr_data_t;
    size_t attr_size = 0;

    // By default we set the fillvalue to 0
    memset(this->ptr_fill_value, 0, FILL_VALUE_SIZE);

    ptr_ndattr = ptr_attr_list->find(ATTR_FILL_VAL);
    // Check whether the attribute exist in the list
    if (ptr_ndattr == NULL) return -1;

    // Get the datatype and size of the attribute
    ptr_ndattr->getValueInfo(&attr_data_t, &attr_size);
    if (attr_data_t > FILL_VALUE_SIZE) return -1;
    ptr_ndattr->getValue(attr_data_t, this->ptr_fill_value, attr_size);
    return 0;
}

/**
 * Parse a a series of attributes formatted: "h5_nnnn_%d"
 */
int WriteConfig::get_attr_array(string& attr_name, NDAttributeList *ptr_attr_list,
                                vec_ds_t& dst)
{
    unsigned int i = 0;
    int retcode = 0;
    int attr_value = 0;
    while (retcode >= 0)
    {
        stringstream ss_attr(stringstream::out);
        ss_attr << attr_name << i;
        retcode = this->get_attr_value(ss_attr.str(), ptr_attr_list, &attr_value);
        //cout << "get_attr_values (" << ss_attr.str() << ")return: " << attr_value << endl;
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
    string attr_name;
    vec_ds_t tmpdims;
    int ret=0;

    // Get the dimensions of the ndarray.
    // By default we set the chunk size and full frame size to be
    // equal to the current frame. Later we override chunk and full size
    // if this information is available in the ndarray attributes.
    this->dim_roi_frame = DimensionDesc(ndarray);
    this->dim_chunk = this->dim_roi_frame;

    // Get the chunking size from the NDArray attributes
    attr_name = ATTR_CHUNK_SIZE;
    tmpdims.clear();
    ret = this->get_attr_array(attr_name, list, tmpdims);
    if (ret > 0) {
        this->dim_chunk = DimensionDesc(tmpdims, this->dim_roi_frame.element_size);
    }

    // Get the full dataset size from the NDArray attributes
    attr_name = ATTR_DSET_SIZE;
    tmpdims.clear();
    ret = this->get_attr_array(attr_name, list, tmpdims);
    if (ret > 0) {
        this->dim_full_dataset = DimensionDesc(tmpdims, this->dim_roi_frame.element_size);
        cout << "full dataset: " << this->dim_full_dataset << endl;
    }

    // Configure the current active dataset with the frame dimensions and the
    // additional dimensions all set to 1
    this->dim_active_dataset = this->get_detector_dims();
    vec_ds_t vec_act_dset = this->dim_active_dataset.dim_size_vec();
    int nextradims = this->dim_full_dataset.num_dimensions() - this->dim_roi_frame.num_dimensions();
    vec_act_dset.insert(vec_act_dset.end(), nextradims, 1);
    this->dim_active_dataset = DimensionDesc(vec_act_dset, this->dim_roi_frame.element_size);
    cout << this->dim_active_dataset << endl;

    /* Collect the fill value from the ndarray attributes */
    ret = this->get_attr_fill_val(list);

    //cout << *this << endl;
}

int WriteConfig::get_attr_value(const string& attr_name, NDAttributeList *ptr_attr_list, int *attr_value)
{
    long int retval = 0;
    int retcode = 0;
    NDAttribute *ptr_ndattr;
    //cout << "getting attribute: " << attr_name << endl;
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

const HSIZE_T * WriteConfig::get_vec_ptr(vec_ds_t& vec)
{
    return (const HSIZE_T*)&vec.front();
}


bool WriteConfig::delay_dim_config()
{
    if (this->dim_roi_frame.num_dimensions() <= 0) return false;
    else return true;
}

/** find out whether or not the input is a prime number.
 * Returns true if number is a prime. False if not.
 */
bool is_prime(unsigned int long number)
{
    //0 and 1 are prime numbers
    if(number == 0 || number == 1) return true;

    //find divisor that divides without a remainder
    int divisor;
    for(divisor = (number/2); (number%divisor) != 0; --divisor){;}

    //if no divisor greater than 1 is found, it is a prime number
    return divisor == 1;
}
