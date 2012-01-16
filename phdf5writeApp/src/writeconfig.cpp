/*
 * writeconfig.cpp
 *
 *  Created on: 11 Jan 2012
 *      Author: up45
 */
#include <iostream>
#include <sstream>
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

WriteConfig::WriteConfig(string& filename, NDArray& ndarray)
:alignment(H5_DEFAULT_ALIGN)
{
    this->_default_init();
    this->str_file_name = filename;
    this->parse_ndarray_attributes(ndarray);
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


string WriteConfig::file_name()
{
    return this->str_file_name;
}

void WriteConfig::get_fill_value(void *fill_value, size_t *max_size_bytes)
{
    size_t num_bytes = FILL_VALUE_SIZE;
    if (num_bytes > *max_size_bytes) num_bytes = *max_size_bytes;
    memcpy(fill_value, this->ptr_fill_value, num_bytes);
    *max_size_bytes = num_bytes;
}

DimensionDesc WriteConfig::min_chunk_cache()
{
    DimensionDesc min_cache_block = this->dim_roi_frame;
    if (min_cache_block.grow_by_block(this->dim_chunk) < 0)
        return this->dim_roi_frame;

    return min_cache_block;
}

/** Calculate the number of 'slots' in the cache. According to HDF5 documentation
 * this is advised to be a prime number of about 10 to 100 times the number of
 * chunks that fit in the cache.
 */
unsigned int WriteConfig::cache_num_slots( DimensionDesc& cache_block )
{

}

/*========================================================================
 *
 * Private method implementations
 *
 * ======================================================================= */


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
                                unsigned long int **dst, size_t max_num_elements)
{
    unsigned int i = 0;
    int size = 0;
    unsigned long int* sizes = *dst;
    for(i=0; i<max_num_elements; i++)
    {
        stringstream ss_attr(stringstream::out);
        ss_attr << attr_name << i;
        size = this->get_attr_value(ss_attr.str(), ptr_attr_list);
        if (size > 0) sizes[i] += (unsigned long)size;
        else break;
    }
    return i-1;
}

/** Parse the attributes of an NDArray and use the information
 * to populate the private data structures.
 *
 */
void WriteConfig::parse_ndarray_attributes(NDArray& ndarray)
{
    NDAttributeList * list = ndarray.pAttributeList;
    string attr_name;
    unsigned long int *tmpdims;
    tmpdims = (unsigned long int*)calloc(ND_ARRAY_MAX_DIMS, sizeof(unsigned long int));
    int ret=0;

    // Get the dimensions of the ndarray.
    // By default we set the chunk size and full frame size to be
    // equal to the current frame. Later we override chunk and full size
    // if this information is available in the ndarray attributes.
    this->dim_roi_frame = DimensionDesc(ndarray);
    this->dim_chunk = this->dim_roi_frame;
    this->dim_full_frame = this->dim_roi_frame;

    attr_name = ATTR_CHUNK_SIZE;
    ret = this->get_attr_array(attr_name, list, &tmpdims, ND_ARRAY_MAX_DIMS);
    if (ret > 0) {
        this->dim_chunk = DimensionDesc(ret, tmpdims, this->dim_roi_frame.element_size);
    }

    attr_name = ATTR_FRAME_SIZE;
    ret = this->get_attr_array(attr_name, list, &tmpdims, ND_ARRAY_MAX_DIMS);
    if (ret > 0) {
        this->dim_full_frame = DimensionDesc(ret, tmpdims, this->dim_roi_frame.element_size);
    }

    attr_name = ATTR_ROI_ORIGIN;
    ret = this->get_attr_array(attr_name, list, &tmpdims, ND_ARRAY_MAX_DIMS);
    if (ret > 0) {
        // Todo: what to do with origins?
    }

    attr_name = ATTR_DSET_SIZE;
    ret = this->get_attr_array(attr_name, list, &tmpdims, ND_ARRAY_MAX_DIMS);
    if (ret > 0) {
        this->dim_full_dataset = DimensionDesc(ret, tmpdims, this->dim_roi_frame.element_size);
    }

    /* Collect the fill value from the ndarray attributes */
    ret = this->get_attr_fill_val(list);

    free(tmpdims);

    cout << this->dim_roi_frame << " ";
    cout << this->dim_chunk << endl;
}

long int WriteConfig::get_attr_value(const string& attr_name, NDAttributeList *ptr_attr_list)
{
    long int retval = 0;
    int retcode = 0;
    NDAttribute *ptr_ndattr;
    cout << "getting attribute: " << attr_name << endl;
    ptr_ndattr = ptr_attr_list->find(attr_name.c_str());
    if (ptr_ndattr == NULL) return -1;
    if (ptr_ndattr->dataType ==  NDAttrString ) return -1;

    retcode = ptr_ndattr->getValue(NDAttrUInt32, (void*)&retval, sizeof(long int));
    if (retcode == ND_ERROR) {
        retcode = ptr_ndattr->getValue(NDAttrInt32, (void*)&retval, sizeof(long int));
    }
    if (retcode == ND_ERROR) return -1;
    return retval;
}

void WriteConfig::_default_init()
{
    this->ptr_fill_value = (void*)calloc(FILL_VALUE_SIZE, sizeof(char));
}

bool WriteConfig::delay_dim_config()
{
    if (this->dim_roi_frame.num_dimensions() <= 0) return false;
    else return true;
}
