/*
 * test_dimension.cpp
 *
 *  Created on: 10 Jan 2012
 *      Author: up45
 *
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

void util_fill_ndarr_dims(NDArray &ndarr, unsigned long int *sizes, int ndims)
{
    static short counter = 3;
    int i=0;
    ndarr.ndims = ndims;
    int num_elements = 1;
    for (i=0; i<ndims; i++) {
    	// Note: the NDArray dimensions are reverse order in relation to
    	//       the HDF5 dataset dimensionality. sigh....
        ndarr.initDimension(&(ndarr.dims[ndims-i-1]), sizes[i]);
        num_elements *= sizes[i];
    }
    ndarr.pData = calloc(num_elements, sizeof(short));
    short *ptrdata = (short*)ndarr.pData;
    ptrdata[0] = counter++; ptrdata[1] = counter++;
}


//============================================================================
/** Suite to test the features of the NDArrayToHDF5 class.
 *
 * Will test constructor, open, write and close functions.
 *
 * The fixture include a set of small NDArray's to write to the file:
 *
 *  Images of 6x4 16bit deep.
 *  ROI splits frame in two (hi/low): 6x2
 */
#define DIMFRAME 0
#define DIMY 1
#define DIMX 2

#define NDIMENSIONS 3
#define NFRAMES 4
#define YSIZE 4
#define XSIZE 6
// full dataset defined as fastest changing dimension last:
// datset dims: [NFRAMES, YSIZE, XSIZE]
//              [      8,     4,     6]
#define CHUNKZ 2
#define CHUNKY 2
#define CHUNKX 6
// Chunk 3 horizontal slices off the frame - and cache n'th frame
// chunking dims: [CHUNKZ, CHUNKY, CHUNKX]
//                [     2,      2,      6]


struct FrameSetFixture{
    std::string fname;
    NDArray hiframe[8];
    NDArray lowframe[8];
    unsigned long int sizes[8], chunks[8], dsetdims[8];
    unsigned long int lowoffset, hioffset, zero;
    int numframes;
    int fill_value;

    MPI_Comm mpi_comm;
    char mpi_name[100];
    int mpi_name_len;
    int mpi_size;
    int mpi_rank;


    FrameSetFixture()
    {
        BOOST_TEST_MESSAGE("setup FrameSetFixture");
        zero=0; hioffset =0;
        fname = "smallframes.h5";
        sizes[0]=YSIZE/2; sizes[1]=XSIZE;
        chunks[DIMFRAME]=CHUNKZ; chunks[DIMY]=CHUNKY; chunks[DIMX]=CHUNKX;
        dsetdims[DIMFRAME]=NFRAMES, dsetdims[DIMY]=YSIZE, dsetdims[DIMX]=XSIZE;
        numframes = NFRAMES;
        fill_value = 4;

        for (int i=0; i<NFRAMES; i++)
        {
        	hiframe[i].dataType = NDUInt16;
        	hiframe[i].dataSize = sizeof(unsigned short);
            util_fill_ndarr_dims( hiframe[i], sizes, 2);
            hiframe[i].pAttributeList->add("h5_fill_val", "fill value", NDAttrUInt32, (void*)&fill_value );

            hiframe[i].pAttributeList->add("h5_chunk_size_0", "dimension 0", NDAttrUInt32, (void*)(chunks) );
            hiframe[i].pAttributeList->add("h5_chunk_size_1", "dimension 1", NDAttrUInt32, (void*)(chunks+1) );
            hiframe[i].pAttributeList->add("h5_chunk_size_2", "dimension 2", NDAttrUInt32, (void*)(chunks+2) );

            hiframe[i].pAttributeList->add("h5_dset_size_0", "dset 0", NDAttrUInt32, (void*)(dsetdims) );
            hiframe[i].pAttributeList->add("h5_dset_size_1", "dset 1", NDAttrUInt32, (void*)(dsetdims+1) );
            hiframe[i].pAttributeList->add("h5_dset_size_2", "dset 2", NDAttrUInt32, (void*)(dsetdims+2) );

            util_fill_ndarr_dims( lowframe[i], sizes, 2);
        	lowframe[i].dataType = NDUInt16;
        	lowframe[i].dataSize = sizeof(unsigned short);
            lowframe[i].pAttributeList->add("h5_fill_val", "fill value", NDAttrUInt32, (void*)&fill_value );
            lowframe[i].pAttributeList->add("h5_chunk_size_0", "dimension 0", NDAttrUInt32, (void*)(chunks) );
            lowframe[i].pAttributeList->add("h5_chunk_size_1", "dimension 1", NDAttrUInt32, (void*)(chunks+1) );
            lowframe[i].pAttributeList->add("h5_chunk_size_2", "dimension 2", NDAttrUInt32, (void*)(chunks+2) );

            lowframe[i].pAttributeList->add("h5_dset_size_0", "dset 0", NDAttrUInt32, (void*)(dsetdims) );
            lowframe[i].pAttributeList->add("h5_dset_size_1", "dset 1", NDAttrUInt32, (void*)(dsetdims+1) );
            lowframe[i].pAttributeList->add("h5_dset_size_2", "dset 2", NDAttrUInt32, (void*)(dsetdims+2) );

            // Add some additional testing NDAttributes
            epicsFloat64 motorpos = 0.1 * i;
            hiframe[i].pAttributeList->add("positions", "A record of motor positions in scan", NDAttrFloat64, (void*)&motorpos);
            lowframe[i].pAttributeList->add("positions", "A record of motor positions in scan", NDAttrFloat64, (void*)&motorpos);

            //frames[i].report(11);
        }

        mpi_size = 0;
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

    }

    void add_attr_origin()
    {
        hioffset = 0;
        lowoffset =2;
        for (int i=0; i<NFRAMES; i++)
        {
            hiframe[i].pAttributeList->add("h5_roi_origin_0", "offset 0", NDAttrUInt32, (void*)&i );
            hiframe[i].pAttributeList->add("h5_roi_origin_1", "offset 1", NDAttrUInt32, (void*)&hioffset );
            hiframe[i].pAttributeList->add("h5_roi_origin_2", "offset 2", NDAttrUInt32, (void*)&zero );

            lowframe[i].pAttributeList->add("h5_roi_origin_0", "offset 0", NDAttrUInt32, (void*)&i );
            lowframe[i].pAttributeList->add("h5_roi_origin_1", "offset 1", NDAttrUInt32, (void*)&lowoffset );
            lowframe[i].pAttributeList->add("h5_roi_origin_2", "offset 2", NDAttrUInt32, (void*)&zero );
        }

    }

    ~FrameSetFixture()
    {
        BOOST_TEST_MESSAGE("teardown FrameSetFixture");
        for (int i=0; i<NFRAMES; i++)
        {
            if (hiframe[i].pData !=NULL ) {
                free( hiframe[i].pData );
                hiframe[i].pData=NULL;
            }
            if (lowframe[i].pData != NULL) {
                free( lowframe[i].pData );
                lowframe[i].pData = NULL;
            }
        }

#ifdef H5_HAVE_PARALLEL
        BOOST_TEST_MESSAGE("==== MPI_Finalize  rank: " << mpi_rank << "=====");
        MPI_Finalize();
#endif

    }
};


/* Single, parallel run
 * Work as an MPI job with RANK=2separate raw data from flatfields, backgrounds etc
 * First process will write the high frames and second process the low frames
 */
BOOST_FIXTURE_TEST_SUITE(ParallelRun, FrameSetFixture)

BOOST_AUTO_TEST_CASE(mpi_parallel_run)
{
    vec_ds_t test_offsets = vec_ds_t(3,0);
    vec_ds_t test_dset_dims = vec_ds_t(3,0);

#ifdef H5_HAVE_PARALLEL

    BOOST_TEST_MESSAGE("BOOST: === TEST SingleParallelRun is parallel ===");
    BOOST_TEST_MESSAGE("BOOST: Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh(mpi_comm, MPI_INFO_NULL);

#else
    BOOST_TEST_MESSAGE("BOOST: === TEST SingleParallelRun is *not* parallel ===");
    BOOST_TEST_MESSAGE("BOOST: Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;
#endif

    WriteConfig wc;
    add_attr_origin();

    NDArray *ndarr;
    if (mpi_rank == 0) {
        ndarr = hiframe;
        test_offsets[1] = 0;
    }
    else {
        ndarr = lowframe;
        test_offsets[1] = 2;
    }

    //ndarr[0].report(11);

    BOOST_REQUIRE_NO_THROW( ndh.h5_configure(ndarr[0]));

    BOOST_TEST_MESSAGE("BOOST: Open file: test_frames_attr_offset_loop.h5");
    BOOST_REQUIRE_EQUAL( ndh.h5_open("test_frames_attr_offset_loop.h5"), 0);

    test_dset_dims[DIMY]=YSIZE; test_dset_dims[DIMX]=XSIZE;
    for (int i = 0; i < NFRAMES; i++) {
        test_dset_dims[DIMFRAME]=i+1;

        BOOST_TEST_MESSAGE("BOOST: Writing frame no: " << i << " rank: " << mpi_rank);
        BOOST_REQUIRE_EQUAL( ndh.h5_write( ndarr[i]), 0);
        test_offsets[DIMFRAME]=i;
        BOOST_REQUIRE_NO_THROW( wc = ndh.get_conf() );
        //BOOST_TEST_MESSAGE( "BOOST: WriteConfig: " << wc._str_() );
        BOOST_REQUIRE( wc.get_offsets() == test_offsets );
        BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    }

    BOOST_TEST_MESSAGE("BOOST: Done writing frame");
    BOOST_REQUIRE_EQUAL( ndh.h5_close(), 0);

}

BOOST_AUTO_TEST_SUITE_END()


struct NoAttrFrameSetFixture{
    std::string fname;
    NDArray hiframe[8];
    NDArray lowframe[8];
    unsigned long int sizes[8], chunks[8], dsetdims[8];
    unsigned long int lowoffset, hioffset, zero;
    int numframes;
    int fill_value;

    MPI_Comm mpi_comm;
    char mpi_name[100];
    int mpi_name_len;
    int mpi_size;
    int mpi_rank;


    NoAttrFrameSetFixture()
    {
        BOOST_TEST_MESSAGE("setup NoAttrFrameSetFixture");
        zero=0;
        fname = "smallframes.h5";
        sizes[0]=6; sizes[1]=2;
        chunks[0]=6; chunks[1]=2; chunks[2]=2;
        dsetdims[0]=6, dsetdims[1]=4, dsetdims[2]=8;
        numframes = 8;
        fill_value = 4;


        for (int i=0; i<8; i++)
        {
            util_fill_ndarr_dims( hiframe[i], sizes, 2);
            util_fill_ndarr_dims( lowframe[i], sizes, 2);
            //frames[i].report(11);
        }

        mpi_size = 0;
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

    }

    ~NoAttrFrameSetFixture()
    {
        BOOST_TEST_MESSAGE("teardown NoAttrFrameSetFixture");
        for (int i=0; i<8; i++)
        {
            if (hiframe[i].pData !=NULL ) {
                free( hiframe[i].pData );
                hiframe[i].pData=NULL;
            }
            if (lowframe[i].pData != NULL) {
                free( lowframe[i].pData );
                lowframe[i].pData = NULL;
            }
        }

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
BOOST_FIXTURE_TEST_SUITE(ParallelNoAttr, NoAttrFrameSetFixture)

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

    // One process (0) write the high frame and the other the low part of the frame
    NDArray *ndarr;
    if (mpi_rank == 0) {
        ndarr = hiframe;
        test_offsets[1] = 0;
    }
    else {
        ndarr = lowframe;
        test_offsets[1] = 2;
    }

    //ndarr[0].report(11);

    BOOST_CHECK_NO_THROW( ndh.h5_configure(ndarr[0]));

    BOOST_TEST_MESSAGE("Open file: test_frames_attr_offset_loop.h5");
    BOOST_REQUIRE_EQUAL( ndh.h5_open("test_frames_attr_offset_loop.h5"), 0);

    test_dset_dims[0]=6; test_dset_dims[1]=4;
    for (int i = 0; i < 8; i++) {
        test_dset_dims[2]=i+1;

        BOOST_TEST_MESSAGE("Writing frame no: " << i << " rank: " << mpi_rank);
        BOOST_REQUIRE_EQUAL( ndh.h5_write( ndarr[i]), 0);
        test_offsets[2]=i;
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

