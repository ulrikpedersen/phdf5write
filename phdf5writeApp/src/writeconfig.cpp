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
    out << " filename=" << this->file_name();
    out << " dset n=" << this->dim_full_dataset.num_dimensions();
    out << " roi n=" << this->dim_roi_frame.num_dimensions();
    out << ">";
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
                                vector<unsigned long int>& dst)
{
    unsigned int i = 0;
    int size = 1;
    while (size > 0)
    {
        stringstream ss_attr(stringstream::out);
        ss_attr << attr_name << i;
        size = this->get_attr_value(ss_attr.str(), ptr_attr_list);
        if (size > 0) dst.push_back((unsigned long)size);
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
    vector <unsigned long int> tmpdims;
    int ret=0;

    // Get the dimensions of the ndarray.
    // By default we set the chunk size and full frame size to be
    // equal to the current frame. Later we override chunk and full size
    // if this information is available in the ndarray attributes.
    this->dim_roi_frame = DimensionDesc(ndarray);
    this->dim_chunk = this->dim_roi_frame;
    this->dim_full_frame = this->dim_roi_frame;

    attr_name = ATTR_CHUNK_SIZE;
    ret = this->get_attr_array(attr_name, list, tmpdims);
    if (ret > 0) {
        this->dim_chunk = DimensionDesc(tmpdims, this->dim_roi_frame.element_size);
    }

    attr_name = ATTR_FRAME_SIZE;
    ret = this->get_attr_array(attr_name, list, tmpdims);
    if (ret > 0) {
        this->dim_full_frame = DimensionDesc(tmpdims, this->dim_roi_frame.element_size);
    }

    attr_name = ATTR_ROI_ORIGIN;
    ret = this->get_attr_array(attr_name, list, tmpdims);
    if (ret > 0) {
        // Todo: what to do with origins?
    }

    attr_name = ATTR_DSET_SIZE;
    ret = this->get_attr_array(attr_name, list, tmpdims);
    if (ret > 0) {
        this->dim_full_dataset = DimensionDesc(tmpdims, this->dim_roi_frame.element_size);
    }

    /* Collect the fill value from the ndarray attributes */
    ret = this->get_attr_fill_val(list);

    cout << *this << endl;
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
