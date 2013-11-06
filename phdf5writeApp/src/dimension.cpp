/*
 * dimension.cpp
 *
 *  Created on: 6 Jan 2012
 *      Author: up45
 */

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <NDArray.h>
#include "dimension.h"

namespace phdf5 {

/*================================================================================
    DimensionDesc Class implementation
*/
DimensionDesc::DimensionDesc()
{
    this->element_size = 0;
}

DimensionDesc::DimensionDesc(NDArray& ndarray)
{
    int ndims = ndarray.ndims;
    vec_ds_t tmpdims(ndims);
    this->dims = vec_ds_t(ndims);
    for (int i=0; i < ndims; i++)
    {
        tmpdims[i] = ndarray.dims[i].size;
    }
    reverse_copy(tmpdims.begin(), tmpdims.end(), this->dims.begin());
    //this->dims = tmpdims;

    NDArrayInfo_t arr_info;
    ndarray.getInfo(&arr_info);
    this->element_size = arr_info.bytesPerElement;
}

DimensionDesc::DimensionDesc(int num_dimensions, const dimsize_t * sizes, size_t element_size)
{
    this->dims.clear();
    if (sizes != NULL) this->dims = vec_ds_t(sizes, sizes+num_dimensions);
    else this->dims = vec_ds_t();
    this->element_size = element_size;
}

DimensionDesc::DimensionDesc(const vec_ds_t& sizes, size_t element_size)
{
    this->dims.clear();
    this->dims = sizes;
    this->element_size = element_size;
}

DimensionDesc::DimensionDesc( const DimensionDesc& src)
{
    this->copy(src);
}

DimensionDesc::~DimensionDesc()
{
    this->dims.clear();
}

DimensionDesc& DimensionDesc::operator =(const DimensionDesc& src)
{
    // Check for self-assignment
    if (this == &src)
        return *this;
    // perform the copy
    this->copy(src);
    return *this;
}

DimensionDesc& DimensionDesc::operator +=(const dimsize_t dimsize)
{
    this->dims.insert(this->dims.begin(), dimsize);
    return *this;
}
DimensionDesc& DimensionDesc::operator +=(const DimensionDesc& src)
{
    vec_ds_t srcdims = src.dims;
    this->dims.insert(this->dims.begin(), srcdims.begin(), srcdims.end());
    return *this;
}

bool DimensionDesc::operator==(const DimensionDesc& compare)
{
    return this->is_equal(compare);
}
bool DimensionDesc::operator!=(const DimensionDesc& compare)
{
    return not this->is_equal(compare);
}


/** Compare two DimensionDesc objects. Check the number of dimensions
 *  as well as each dimension size
 */
bool DimensionDesc::is_equal(const DimensionDesc& compare)
{
    if (this->dims.size() != compare.dims.size())
        return false;
    return std::equal(this->dims.begin(), this->dims.end(), compare.dims.begin());
}

/** Copy the src into the private data of this object.
 * Essentially a deep-copy from src to this object.
 */
void DimensionDesc::copy( const DimensionDesc& src)
{
    this->dims.clear();
    this->dims = vec_ds_t(src.dims);
    this->element_size = src.element_size;
}


std::string DimensionDesc::DimensionDesc::_str_()
{
    std::stringstream out(std::stringstream::out);
    out << "<DimensionDesc:";
    out << " n="<< this->dims.size();
    out << " [ ";
    vec_ds_t::const_iterator it;
    for (it = this->dims.begin(); it != this->dims.end(); ++it)
    {
        out << *it << ", ";
    }
    out << "]";
    out << " elements=" << this->data_num_elements();
    out << " bytes=" << this->data_num_bytes();
    out << ">";
    return out.str();
}

int DimensionDesc::set_dimension_size(unsigned int dimension, dimsize_t size)
{
    if (dimension >= this->dims.size()) return -1;
    this->dims[dimension] = size;
    return 0;
}

int DimensionDesc::grow_by_block(DimensionDesc& block)
{
    double div_result = 0.0;
    double block_size = 0.0, this_size=0.0;

    // sanity check: the container must have the same number of dimensions as this object...
    // This may be improved later so that a container with more dimensions can be checked.
    if (block.num_dimensions() != this->num_dimensions()) return -1;

    vec_ds_t::iterator it_this;
    vec_ds_t::const_iterator it_block;
    for (it_this=this->dims.begin(), it_block=block.dims.begin();
            it_this != this->dims.end();
            ++it_this, ++it_block)
    {
        block_size = (double)*it_block;
        this_size = (double)*it_this;
        div_result = ceil(this_size/block_size);
        *it_this = (dimsize_t)(div_result * block_size);
    }
    return 0;
}

const dimsize_t * DimensionDesc::dim_sizes()
{
    const dimsize_t * ptr = this->dims.data();
    return ptr;
}

vec_ds_t DimensionDesc::dim_size_vec()
{
    return this->dims;
}

size_t DimensionDesc::data_num_bytes()
{
    size_t num_bytes = 0;
    num_bytes = this->data_num_elements() * this->element_size;
    return num_bytes;
}

long int DimensionDesc::data_num_elements()
{
    long int num_elements = 1;

    // in case no dimensions are defined we simply return zero.
    if (this->dims.size() <= 0) return 0;

    vec_ds_t::const_iterator it;
    for (it = this->dims.begin(); it != this->dims.end(); ++it)
    {
    	if (*it > 0) num_elements *= *it;
    }
    return num_elements;
}

int DimensionDesc::num_dimensions()
{
    return this->dims.size();
}

/** return the number of multiplications of this block fits inside
 (and possibly overlap if overlap==true) the container.
*/
unsigned int DimensionDesc::num_fits(DimensionDesc &container, bool overlap)
{
    unsigned int result = 1;
    double div_result = 0.0;
    vec_ds_t tmpdims = this->dims;

    // sanity check: the container cannot have a smaller number of dimensions than this object...
    if (container.num_dimensions() < this->num_dimensions()) {
        std::cerr << "### ERROR: num fits problem: container:" << container;
        std::cerr << " this: " << *this << std::endl;
        return -1;
    } else {
        // for each dimension that the container is bigger than this->dims we
        // add an additional dimension of size 1.
        // So tmpdims will get the same number of dimensions as the container.
        while ((int)tmpdims.size() < container.num_dimensions())
            tmpdims.push_back(1);
    }
    //cout << "tmp block size: " << tmpdims.size() << " container size: " << container.num_dimensions() << endl;
    vec_ds_t::const_iterator it_this, it_cont;
    for (it_this=tmpdims.begin(), it_cont=container.dims.begin();
            it_this != tmpdims.end();
            ++it_this, ++it_cont)
    {
        div_result = (double)*it_cont / (double)*it_this;

        if (overlap) div_result = ceil(div_result);
        else div_result = floor(div_result);

        result *= (int)div_result;
        //cout << "div_result: " << result << endl;
    }
    return result;
}

/*==============  End DimensionDesc Class  =========================================*/



int test_dimensiondesc_simple()
{
//    NDDimension_t nddim[3] = {{4,0,0,0},
//                              {6,0,0,0},
//                              {7,0,0,0}};
    int nddim[3] = {3,6,7};
    int fits = 0;
    NDArray ndarr;
    dimsize_t sizes[3] = {8,12,14};

    ndarr.dataType = NDUInt16;
    ndarr.ndims = 3;
    for (int i=0; i<3; i++) ndarr.initDimension(&(ndarr.dims[i]), nddim[i]);

    ndarr.report(2);

    DimensionDesc *dim1 = new DimensionDesc(ndarr);
    std::cout << *dim1 << std::endl;
    DimensionDesc *dim2 = new DimensionDesc(3, sizes, sizeof(int));
    std::cout << *dim2 << std::endl;

    fits = dim1->num_fits(*dim2, true);
    std::cout << "dim1 fits " << fits << " times in dim2" << std::endl;

    // use of copy constructor
    std::cout << "using copy constructor" << std::endl;
    DimensionDesc *dim3 =  new DimensionDesc(*dim2);
    std::cout << *dim3 << std::endl;

    // Delete the original object and check the copied ones still work
    // (i.e. a real deep copy has been performed rather than just a shallow one)
    std::cout << "deleting the original object" << std::endl;
    delete dim2;
    std::cout << *dim3 << std::endl;

    // use of the assignment operator
    std::cout << "using the assingment operator to assign dim1 to dim3" << std::endl;
    *dim3 = *dim1;
    std::cout << *dim3 << std::endl;

    std::cout << "deleting the original dim1" << std::endl;
    delete dim1;
    std::cout << *dim3 << std::endl;

    delete dim3;
    return 0;
}

} // phdf5



