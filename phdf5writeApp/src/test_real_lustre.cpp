/*
 * test_real_lustre.cpp
 *
 *  Created on: 14 Feb 2012
 *      Author: up45
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MPI_NDArrayToHDF5
#include <boost/test/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <exception>
#include <NDArray.h>
#include "ndarray_hdf5.h"

typedef struct dims_t{
    int x;
    int y;
    int z;
} dims_t;

struct sim_config
{
    std::string filename;
    dims_t chunking;
    dims_t subframe;
    int num_frames;
    int num_chunks;
    int fill_value;
    void loadxml(const std::string &xmlfile);
};

void sim_config::loadxml(const std::string &xmlfile)
{

    // Create empty property tree object
    using boost::property_tree::ptree;
    ptree pt;

    // Load XML file and put its contents in property tree.
    read_xml(xmlfile, pt);

    // Get the output filename and store it.
    filename = pt.get<std::string>("phdftest.filename");

    // Get chunking settings
    chunking.x = pt.get("phdftest.chunking.x", 1);
    chunking.y = pt.get("phdftest.chunking.y", 1);
    chunking.z = pt.get("phdftest.chunking.z", 1);

    // get the subframe dimensions -i.e. the size of the stripe that the current process will write.
    // Each process will write with an offset in the Y dimension, depending on their mpi-rank (process ID)
    subframe.x = pt.get("phdftest.subframe.x", 1);
    subframe.y = pt.get("phdftest.subframe.y", 1);
    subframe.z = pt.get("phdftest.subframe.z", 1);

    // Get the number of frames to write
    num_frames = pt.get("phdftest.numframes", 1);

    // fill value
    fill_value = pt.get("phdftest.fillvalue", 0);

}

//#define PCO_EDGE_TEST
//#define PCO_4000_TEST
//#define EVEN_CHUNKS_TEST
//#define TINY_TEST


#ifdef EVEN_CHUNKS_TEST
#define NUM_FRAMES 600
#define CHUNK_X 4096
#define CHUNK_Y 64
#define CHUNK_Z 8
#define NUM_CHUNKS 20
#endif

#ifdef TINY_TEST
#define NUM_FRAMES 6
#define CHUNK_X 10
#define CHUNK_Y 2
#define CHUNK_Z 2
#define NUM_CHUNKS 3
#endif

void util_fill_ndarr_dims(NDArray &ndarr, unsigned long int *sizes, int ndims, int rank)
{
    static short counter=0;
    int i=0;
    ndarr.ndims = ndims;
    ndarr.dataType = NDUInt16;
    int num_elements = 1;
    for (i=0; i<ndims; i++) {
        ndarr.initDimension(&(ndarr.dims[i]), sizes[i]);
        num_elements *= sizes[i];
    }
    ndarr.pData = calloc(num_elements, sizeof(short));
    short *ptrdata = (short*)ndarr.pData;
    ptrdata[0] = rank; ptrdata[1] = ++counter;
}


struct WriteFrameFixture{
    std::string fname;
    std::vector<NDArray *> frames;
    unsigned long int sizes[2], chunks[3], dsetdims[3];
    int numframes;
    int fill_value;

    MPI_Comm mpi_comm;
    char mpi_name[100];
    int mpi_name_len;
    int mpi_size;
    int mpi_rank;
    sim_config config;


    WriteFrameFixture()
    {
        BOOST_TEST_MESSAGE("setup WriteFrameFixture");
        //fname = "smallframes.h5";
        config.loadxml("testconf.xml");

        mpi_size = 1;
        mpi_rank = 0;
        mpi_name_len = 100;
#ifdef H5_HAVE_PARALLEL

        mpi_comm = MPI_COMM_WORLD;

        MPI_Init(NULL, NULL);

        MPI_Comm_size(mpi_comm, &mpi_size);
        MPI_Comm_rank(mpi_comm, &mpi_rank);

        MPI_Get_processor_name(mpi_name, &mpi_name_len);
        BOOST_TEST_MESSAGE("=== TEST is parallel ===");
        BOOST_TEST_MESSAGE("  MPI size: " << mpi_size << " MPI rank: " << mpi_rank << " host: " << mpi_name );


#endif

        // output filename
        fname = config.filename;

        // Define the dimensions and number of frames to run here
        //numframes = NUM_FRAMES;
        numframes = config.num_frames;
        BOOST_TEST_MESSAGE("num_frames: " << numframes);

        //sizes[0]=CHUNK_X; sizes[1]=CHUNK_Y * NUM_CHUNKS;
        sizes[0] = config.subframe.x; sizes[1] = config.subframe.y;

        //chunks[0]=CHUNK_X; chunks[1]=CHUNK_Y; chunks[2]=CHUNK_Z;
        chunks[0] = config.chunking.x; chunks[1] = config.chunking.y; chunks[2] = config.chunking.z;

        dsetdims[0]=sizes[0], dsetdims[1]=sizes[1]*mpi_size, dsetdims[2]=numframes;

        BOOST_TEST_MESSAGE("dataset: " << dsetdims[0] << " " << dsetdims[1] << " " << dsetdims[2]);
        fill_value = config.fill_value;

        BOOST_TEST_MESSAGE("chunks: " << chunks[0] << " " << chunks[1] << " " << chunks[2]);
        BOOST_TEST_MESSAGE("fill value: " << fill_value);

        for (int i=0; i<chunks[2]; i++)
        {
            NDArray *pnd = new NDArray();
            pnd->pAttributeList->add("h5_chunk_size_0", "dimension 0", NDAttrUInt32, (void*)(chunks) );
            pnd->pAttributeList->add("h5_chunk_size_1", "dimension 1", NDAttrUInt32, (void*)(chunks+1) );
            pnd->pAttributeList->add("h5_chunk_size_2", "dimension 2", NDAttrUInt32, (void*)(chunks+2) );

            pnd->pAttributeList->add("h5_dset_size_0", "dset 0", NDAttrUInt32, (void*)(dsetdims) );
            pnd->pAttributeList->add("h5_dset_size_1", "dset 1", NDAttrUInt32, (void*)(dsetdims+1) );
            pnd->pAttributeList->add("h5_dset_size_2", "dset 2", NDAttrUInt32, (void*)(dsetdims+2) );

            pnd->pAttributeList->add("h5_fill_val", "fill value", NDAttrUInt32, (void*)(&fill_value) );
            frames.push_back( pnd );
            util_fill_ndarr_dims( *frames[i], sizes, 2, mpi_rank);

            pnd->report(11);
        }
    }

    ~WriteFrameFixture()
    {
        BOOST_TEST_MESSAGE("teardown WriteFrameFixture");
        std::vector<NDArray*>::const_iterator it;
        NDArray* pnd;
        for (it = frames.begin(); it != frames.end(); ++it)
        {
            pnd = *it;
            if (pnd->pData !=NULL ) {
                BOOST_TEST_MESSAGE("Freeing address: " << pnd->pData << " from: " << &pnd);
                free(pnd->pData );
                pnd->pData=NULL;
            }
        }
        frames.clear();

#ifdef H5_HAVE_PARALLEL
        BOOST_TEST_MESSAGE("==== MPI_Finalize  rank: " << mpi_rank << "=====");
        MPI_Finalize();
#endif

    }
};


/* Single, parallel run with no NDAttributes to guide the WriteConfig
 * Work as an MPI job with RANK=2
 * First process will write the high frames and second process the low frames
 */
BOOST_FIXTURE_TEST_SUITE(PerformanceTest, WriteFrameFixture)

BOOST_AUTO_TEST_CASE(mpi_parallel_run)
{
    vec_ds_t test_offsets = vec_ds_t(3,0);
    vec_ds_t test_dset_dims = vec_ds_t(3,0);

#ifdef H5_HAVE_PARALLEL

    BOOST_TEST_MESSAGE("=== TEST ParallelNoAttr is parallel ===");
    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh(mpi_comm, MPI_INFO_NULL);

#else
    BOOST_TEST_MESSAGE("=== TEST ParallelNoAttr is *not* parallel ===");
    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;
#endif

    WriteConfig wc;

    test_offsets[1] = mpi_rank * sizes[1];

    BOOST_CHECK_NO_THROW( ndh.h5_configure(*frames[0]));

    BOOST_TEST_MESSAGE("Open file: " << fname);
    BOOST_REQUIRE_EQUAL( ndh.h5_open(fname.c_str()), 0);

    test_dset_dims[0]=dsetdims[0]; test_dset_dims[1]=dsetdims[1];
    int cacheframe = 0;

    for (int i = 0; i < numframes; i++) {
        test_dset_dims[2]=i+1;
        test_offsets[2]=i;

        cacheframe = i%frames.size();
        BOOST_TEST_MESSAGE("===== Writing frame[" << cacheframe << "] no: " << i << " rank: " << mpi_rank);

        BOOST_REQUIRE_EQUAL( ndh.h5_write( *frames[cacheframe]), 0);

        wc = ndh.get_conf();
        BOOST_CHECK_EQUAL( wc.get_offsets()[0], test_offsets[0] );
        BOOST_CHECK_EQUAL( wc.get_offsets()[1], test_offsets[1] );
        BOOST_CHECK_EQUAL( wc.get_offsets()[2], test_offsets[2] );
        BOOST_CHECK_EQUAL( wc.get_dset_dims()[0], test_dset_dims[0] );
        BOOST_CHECK_EQUAL( wc.get_dset_dims()[1], test_dset_dims[1] );
        BOOST_CHECK_EQUAL( wc.get_dset_dims()[2], test_dset_dims[2] );

    }

    BOOST_TEST_MESSAGE("Closing file");
    BOOST_CHECK_EQUAL( ndh.h5_close(), 0);

}

BOOST_AUTO_TEST_SUITE_END()

