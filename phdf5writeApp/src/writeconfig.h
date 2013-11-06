/*
 * writeconfig.h
 *
 *  Created on: 11 Jan 2012
 *      Author: up45
 */

#ifndef WRITECONFIG_HPP_
#define WRITECONFIG_HPP_
#include <cstdlib>
#include <string>
#include <log4cxx/logger.h>

#include "dimension.h"

class NDArray; // forward declaration
class NDAttributeList;

namespace phdf5 {

typedef unsigned long long HSIZE_T;

// Some default parameters
#define H5_DEFAULT_ALIGN 1048576
#define H5_DEFAULT_ISTOREK 16000

class WriteConfig {
public:

    // Constants
    static const std::string ATTR_CHUNK_SIZE;
    static const std::string ATTR_ROI_ORIGIN;
    static const std::string ATTR_DSET_SIZE;
    static const std::string ATTR_FILL_VAL;
    static const std::string ATTR_COMPRESSION;

    // Constructors
    WriteConfig();
    WriteConfig(NDArray& ndarray, int mpi_rank=0, int mpi_proc=1);
    WriteConfig(const WriteConfig& src); /** Copy constructor */
    ~WriteConfig();

    // Operators
    WriteConfig& operator=(const WriteConfig& src); /** assignment operator copies src */

    static HSIZE_T *get_vec_ptr(vec_ds_t& vec);

    vec_ds_t get_chunk_dims(){return this->dim_chunk.dim_size_vec();};
    vec_ds_t get_dset_dims(){return this->dim_active_dataset.dim_size_vec();};
    vec_ds_t get_offsets(){return this->origin;};
    vec_ds_t get_roi_frame(){return this->dim_roi_frame.dim_size_vec();};
    vec_ds_t get_dset_maxdims();
    int get_alloc_time();
    void proc_rank_size(int rank, int size);
    int num_extra_dims();
    long int num_frames();

    // a few flags to turn on/off various IO parameters
    void dset_extendible(bool ext) {this->extendible = ext;};
    void io_mpiposix(bool posix){this->mpiposix = posix;};
    void io_collective(bool collective){this->iocollective = collective;};
    bool is_dset_extendible(){return this->extendible; };
    bool is_io_mpiposix() {return this->mpiposix; };
    bool is_io_collective() {return this->iocollective; };
    void set_alloc_time( int alloc_time) { this->alloc_time = alloc_time; };

    unsigned int istorek();

    HSIZE_T get_alignment();
    DimensionDesc min_chunk_cache();
    unsigned long int cache_num_slots( DimensionDesc& cache_block );
    void get_fill_value(void *fill_value, size_t *max_size_bytes);

    std::string file_name();
    std::string file_name(const char *filename) { this->str_file_name = filename; return this->str_file_name; };
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
    log4cxx::LoggerPtr log;
    void _default_init();
    void _copy(const WriteConfig& src);
    DimensionDesc get_detector_dims();
    int get_attr_fill_val(NDAttributeList *ptr_attr_list);
    int get_attr_array(std::string& attr_name,
                       NDAttributeList *ptr_attr_list,
                       vec_ds_t& dst);
    void parse_ndarray_attributes(NDArray& ndarray); // go through a list of attributes and turn them into dimensions
    int get_attr_value(const std::string& attr_name, NDAttributeList *ptr_attr_list, int *attr_value);

    bool extendible;
    bool mpiposix;
    bool iocollective;
    HSIZE_T alignment;
    int alloc_time;

    int proc_rank;
    int proc_size;
    DimensionDesc dim_chunk;
    DimensionDesc dim_roi_frame;
    DimensionDesc dim_full_dataset;
    DimensionDesc dim_active_dataset;
    vec_ds_t origin;

    // Datatype?
    void * ptr_fill_value;
    std::string str_file_name;
};

// utility functions
bool is_prime(unsigned int long number);

} // phdf5

#endif /* WRITECONFIG_HPP_ */
