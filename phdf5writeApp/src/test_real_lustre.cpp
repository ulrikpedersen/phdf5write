/*
 * test_real_lustre.cpp
 *
 *  Created on: 14 Feb 2012
 *      Author: up45
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MPI_NDArrayToHDF5
#include <boost/test/unit_test.hpp>
//#include <boost/test/included/unit_test_framework.hpp> // for a static build use this and comment out BOOST_TEST_DYN_LINK

#include <iostream>
#include <vector>
#include <string>
#include <NDArray.h>
#include "ndarray_hdf5.h"

//#define PCO_EDGE_TEST
//#define PCO_4000_TEST
#define EVEN_CHUNKS_TEST
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


    WriteFrameFixture()
    {
        BOOST_TEST_MESSAGE("setup WriteFrameFixture");
        fname = "smallframes.h5";

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

        // Define the dimensions and number of frames to run here
        numframes = NUM_FRAMES;
        sizes[0]=CHUNK_X; sizes[1]=CHUNK_Y * NUM_CHUNKS;
        chunks[0]=CHUNK_X; chunks[1]=CHUNK_Y; chunks[2]=CHUNK_Z;
        dsetdims[0]=sizes[0], dsetdims[1]=sizes[1]*mpi_size, dsetdims[2]=numframes;
        BOOST_TEST_MESSAGE("dataset: " << dsetdims[0] << " " << dsetdims[1] << " " << dsetdims[2]);
        fill_value = 4;

        for (int i=0; i<CHUNK_Z; i++)
        {
            NDArray *pnd = new NDArray();
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
                free(pnd->pData );
                pnd->pData=NULL;
            }
        }
        //delete frames;

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

    BOOST_TEST_MESSAGE("Open file: test_real_lustre2.h5");
    BOOST_REQUIRE_EQUAL( ndh.h5_open("test_real_lustre2.h5"), 0);

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

