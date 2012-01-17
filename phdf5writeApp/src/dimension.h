/*
 * dimension.h
 *
 *  Created on: 6 Jan 2012
 *      Author: up45
 */

#ifndef DIMENSION_H_
#define DIMENSION_H_

#include <vector>
#include <string>

#include <cstdlib>
#include <iostream>

class NDArray; // forward declaration

// Describe an N-dimension blob.
// A convenience class to keep track of dimension sizes, names and calculations
class DimensionDesc {
public:
    DimensionDesc();                          /** Default constructor */
    DimensionDesc(int num_dimensions,
                  const unsigned long int *sizes,
                  size_t element_size=sizeof(short));   /** Constructor based on dimension arrays */
    DimensionDesc(const std::vector<unsigned long int>& sizes,
                  size_t element_size=sizeof(short));   /** Constructor based on a vector of dimension sizes */
    DimensionDesc(NDArray& ndarr);                      /** NDArray constructor */
    DimensionDesc( const DimensionDesc& src);           /** copy constructor */
    ~DimensionDesc();
    DimensionDesc& operator=(const DimensionDesc& src); /** Assignment operator */
    DimensionDesc& operator+=(const unsigned long int dimsize);
    DimensionDesc& operator+=(const DimensionDesc& src);
    bool operator==(const DimensionDesc& compare);
    bool operator!=(const DimensionDesc& compare);

    /** Stream operator: use to prints a string representation of this class */
    inline friend std::ostream& operator<<(std::ostream& out, DimensionDesc& dim) {
        out << dim._str_();
        return out;
    }
    std::string _str_();  /** Return a string representation of the object */

    int grow_by_block(DimensionDesc& block); /** Grow the dimensions in multiple of the input block */
    const unsigned long int * dim_sizes();   /** return an array of the size of each dimension */
    size_t data_num_bytes();       /** return the total number of data bytes in the dataset */
    long int data_num_elements();  /** return the total number of data elements in the dataset */
    int num_dimensions();          /** Return the number of dimensions in the block (rank) */

    int num_fits(DimensionDesc &container, bool overlap = false); /** Calculate the number of times this object fits inside the container */
    size_t element_size; // number of bytes per element in the data blob

private:
    bool is_equal(const DimensionDesc& compare);
    void copy(const DimensionDesc& src);

    // list of dimension sizes
    std::vector<unsigned long int> dims;
    // list of dimension names
};

int test_dimensiondesc_simple();


#endif /* DIMENSION_H_ */
