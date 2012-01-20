/*
 * writeconfig.hpp
 *
 *  Created on: 11 Jan 2012
 *      Author: up45
 */

#ifndef WRITECONFIG_HPP_
#define WRITECONFIG_HPP_
#include <cstdlib>
#include <string>

#include "dimension.h"

#define ATTR_CHUNK_SIZE  "h5_chunk_size_"
#define ATTR_ROI_ORIGIN  "h5_roi_origin_"
#define ATTR_DSET_SIZE   "h5_dset_size_"
#define ATTR_FILL_VAL    "h5_fill_val"
#define ATTR_COMPRESSION "h5_compression"

typedef unsigned long long HSIZE_T;

// Some default parameters
#define H5_DEFAULT_ALIGN 1048576
#define H5_DEFAULT_ISTOREK 16000

class NDArray; // forward declaration
class NDAttributeList;

class WriteConfig {
public:
    WriteConfig();
    WriteConfig(std::string& filename);
    WriteConfig(std::string& filename, NDArray &ndarray);
    ~WriteConfig();

    vec_ds_t get_chunk_dims(){return this->dim_chunk.dim_size_vec();};
    vec_ds_t get_dset_dims(){return this->dim_active_dataset.dim_size_vec();};
    vec_ds_t get_offsets(){return this->origin;};

    long int istorek();
    const HSIZE_T alignment;
    DimensionDesc min_chunk_cache();
    unsigned long int cache_num_slots( DimensionDesc& cache_block );
    void get_fill_value(void *fill_value, size_t *max_size_bytes);

    std::string file_name();
    bool delay_dim_config();
    void inc_position(NDArray& ndarray);
    int next_frame(NDArray& ndarray);

    /** Stream operator: use to prints a string representation of this class */
    inline friend std::ostream& operator<<(std::ostream& out, WriteConfig& wc) {
        out << wc._str_();
        return out;
    }
    std::string _str_();  /** Return a string representation of the object */

private:
    void _default_init();
    DimensionDesc get_detector_dims();
    int get_attr_fill_val(NDAttributeList *ptr_attr_list);
    int get_attr_array(std::string& attr_name,
                       NDAttributeList *ptr_attr_list,
                       vec_ds_t& dst);
    void parse_ndarray_attributes(NDArray& ndarray); // go through a list of attributes and turn them into dimensions
    int get_attr_value(const std::string& attr_name, NDAttributeList *ptr_attr_list, int *attr_value);

    DimensionDesc dim_chunk;
    DimensionDesc dim_roi_frame;
    DimensionDesc dim_full_dataset;
    DimensionDesc dim_active_dataset; // TODO: size of dataset with content.
    vec_ds_t origin;

    // Datatype?
    void * ptr_fill_value;
    std::string str_file_name;
};

// utility functions
bool is_prime(unsigned int long number);

#endif /* WRITECONFIG_HPP_ */
