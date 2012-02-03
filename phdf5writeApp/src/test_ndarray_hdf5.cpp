/*
 * test_dimension.cpp
 *
 *  Created on: 10 Jan 2012
 *      Author: up45
 *
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE NDArrayToHDF5Class
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
        ndarr.initDimension(&(ndarr.dims[i]), sizes[i]);
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
 *  Chunking of 6x2x2  (i.e. two ROIs in cache)
 *  Full dataset: 8 full frames (6x4x8)
 */


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
        BOOST_TEST_MESSAGE("setup DynamicFixture");
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
            hiframe[i].pAttributeList->add("h5_fill_val", "fill value", NDAttrUInt32, (void*)&fill_value );

            hiframe[i].pAttributeList->add("h5_chunk_size_0", "dimension 0", NDAttrUInt32, (void*)(chunks) );
            hiframe[i].pAttributeList->add("h5_chunk_size_1", "dimension 1", NDAttrUInt32, (void*)(chunks+1) );
            hiframe[i].pAttributeList->add("h5_chunk_size_2", "dimension 2", NDAttrUInt32, (void*)(chunks+2) );

            hiframe[i].pAttributeList->add("h5_dset_size_0", "dset 0", NDAttrUInt32, (void*)(dsetdims) );
            hiframe[i].pAttributeList->add("h5_dset_size_1", "dset 1", NDAttrUInt32, (void*)(dsetdims+1) );
            hiframe[i].pAttributeList->add("h5_dset_size_2", "dset 2", NDAttrUInt32, (void*)(dsetdims+2) );

            util_fill_ndarr_dims( lowframe[i], sizes, 2);
            lowframe[i].pAttributeList->add("h5_chunk_size_0", "dimension 0", NDAttrUInt32, (void*)(chunks) );
            lowframe[i].pAttributeList->add("h5_chunk_size_1", "dimension 1", NDAttrUInt32, (void*)(chunks+1) );
            lowframe[i].pAttributeList->add("h5_chunk_size_2", "dimension 2", NDAttrUInt32, (void*)(chunks+2) );

            lowframe[i].pAttributeList->add("h5_dset_size_0", "dset 0", NDAttrUInt32, (void*)(dsetdims) );
            lowframe[i].pAttributeList->add("h5_dset_size_1", "dset 1", NDAttrUInt32, (void*)(dsetdims+1) );
            lowframe[i].pAttributeList->add("h5_dset_size_2", "dset 2", NDAttrUInt32, (void*)(dsetdims+2) );

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
        for (int i=0; i<8; i++)
        {
            hiframe[i].pAttributeList->add("h5_roi_origin_0", "offset 0", NDAttrUInt32, (void*)&zero );
            hiframe[i].pAttributeList->add("h5_roi_origin_1", "offset 1", NDAttrUInt32, (void*)&hioffset );
            hiframe[i].pAttributeList->add("h5_roi_origin_2", "offset 2", NDAttrUInt32, (void*)&i );

            lowframe[i].pAttributeList->add("h5_roi_origin_0", "offset 0", NDAttrUInt32, (void*)&zero );
            lowframe[i].pAttributeList->add("h5_roi_origin_1", "offset 1", NDAttrUInt32, (void*)&lowoffset );
            lowframe[i].pAttributeList->add("h5_roi_origin_2", "offset 2", NDAttrUInt32, (void*)&i );
        }

    }

    ~FrameSetFixture()
    {
        BOOST_TEST_MESSAGE("teardown DynamicFixture");
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

BOOST_FIXTURE_TEST_SUITE(SingleRun, FrameSetFixture)
/** Send 8 NDArray frames, split in a high/low half through the NDArrayToHDF5 class.
 * The frames are sent through sequentially. This test is intended to demonstrate the
 * normal use of the system.
 */
BOOST_AUTO_TEST_CASE(frames_attr_offset_loop)
{
    vec_ds_t test_offsets = vec_ds_t(3,0);
    vec_ds_t test_dset_dims = vec_ds_t(3,0);
    WriteConfig wc;
    add_attr_origin();

    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;
    BOOST_CHECK_NO_THROW( ndh.h5_configure(hiframe[0]));

    BOOST_TEST_MESSAGE("Open file: test_frames_attr_offset_loop.h5");
    BOOST_REQUIRE_EQUAL( ndh.h5_open("test_frames_attr_offset_loop.h5"), 0);

    test_dset_dims[0]=6; test_dset_dims[1]=4;
    for (int i = 0; i < 8; i++) {
        test_dset_dims[2]=i+1;

        BOOST_TEST_MESSAGE("Writing frame high " << i);
        BOOST_REQUIRE_EQUAL( ndh.h5_write( hiframe[i]), 0);
        test_offsets[1]=0; test_offsets[2]=i;
        wc = ndh.get_conf();
        BOOST_CHECK( wc.get_offsets() == test_offsets );
        BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

        BOOST_TEST_MESSAGE("Writing frame low " << i);
        BOOST_REQUIRE_EQUAL( ndh.h5_write( lowframe[i]), 0);
        test_offsets[1]=2; test_offsets[2]=i;
        wc = ndh.get_conf();
        BOOST_CHECK( wc.get_offsets() == test_offsets );
        BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );
    }

    BOOST_TEST_MESSAGE("Closing file");
    BOOST_CHECK_EQUAL( ndh.h5_close(), 0);

}
BOOST_AUTO_TEST_SUITE_END()

/* Single, parallel run
 * Work as an MPI job with RANK=2
 * First process will write the high frames and second process the low frames
 */
BOOST_FIXTURE_TEST_SUITE(SingleParallelRun, FrameSetFixture)
BOOST_AUTO_TEST_CASE(frames_attr_offset_loop)
{
    vec_ds_t test_offsets = vec_ds_t(3,0);
    vec_ds_t test_dset_dims = vec_ds_t(3,0);

#ifdef H5_HAVE_PARALLEL

    BOOST_TEST_MESSAGE("=== TEST SingleParallelRun is parallel ===");
    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh(mpi_comm, MPI_INFO_NULL);

#else
    BOOST_TEST_MESSAGE("=== TEST SingleParallelRun is *not* parallel ===");
    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;
#endif

    WriteConfig wc;
    add_attr_origin();

    NDArray* ndarr;
    if (mpi_rank == 0) ndarr = hiframe;
    else ndarr = lowframe;

    BOOST_CHECK_NO_THROW( ndh.h5_configure(ndarr[0]));

    BOOST_TEST_MESSAGE("Open file: test_frames_attr_offset_loop.h5");
    BOOST_REQUIRE_EQUAL( ndh.h5_open("test_frames_attr_offset_loop.h5"), 0);

    test_dset_dims[0]=6; test_dset_dims[1]=4;
    for (int i = 0; i < 8; i++) {
        test_dset_dims[2]=i+1;

        BOOST_TEST_MESSAGE("Writing frame no: " << i << " rank: " << mpi_rank);
        BOOST_REQUIRE_EQUAL( ndh.h5_write( ndarr[i]), 0);
        test_offsets[1]=0; test_offsets[2]=i;
        wc = ndh.get_conf();
        BOOST_CHECK( wc.get_offsets() == test_offsets );
        BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    }

    BOOST_TEST_MESSAGE("Closing file");
    BOOST_CHECK_EQUAL( ndh.h5_close(), 0);

}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(FrameSet, FrameSetFixture)

/** Send 3 frames (first 3 of an 8 frame dataset) through the system.
 * This is just a small test to indicate whether the basic operation works or not.
 */
BOOST_AUTO_TEST_CASE(frames_attr_offset)
{
    vec_ds_t test_offsets;
    vec_ds_t test_dset_dims = vec_ds_t(3,0);
    WriteConfig wc;
    add_attr_origin();
    test_dset_dims[0]=6; test_dset_dims[1]=4;

    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;

    BOOST_CHECK_NO_THROW( ndh.h5_configure(lowframe[0]) );

    BOOST_TEST_MESSAGE("Open file: test_frames_attr_offset.h5");
    BOOST_CHECK( ndh.h5_open("test_frames_attr_offset.h5") == 0);

    //-------------  First frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 0");
    BOOST_CHECK( ndh.h5_write( hiframe[0]) == 0);
    dimsize_t hi_offsets_0[] = {0,0,0};
    test_offsets = vec_ds_t(hi_offsets_0, hi_offsets_0+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=1;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 0");
    BOOST_CHECK( ndh.h5_write( lowframe[0]) == 0);
    dimsize_t lo_offsets_0[] = {0,2,0};
    test_offsets = vec_ds_t(lo_offsets_0, lo_offsets_0+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=1;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    //-------------  Second frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 1");
    BOOST_CHECK( ndh.h5_write( hiframe[1]) == 0);
    dimsize_t hi_offsets_1[] = {0,0,1};
    test_offsets = vec_ds_t(hi_offsets_1, hi_offsets_1+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=2;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 1");
    BOOST_CHECK( ndh.h5_write( lowframe[1]) == 0);
    dimsize_t lo_offsets_1[] = {0,2,1};
    test_offsets = vec_ds_t(lo_offsets_1, lo_offsets_1+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=2;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    //-------------  Third frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 2");
    BOOST_CHECK( ndh.h5_write( hiframe[2]) == 0);
    dimsize_t hi_offsets_2[] = {0,0,2};
    test_offsets = vec_ds_t(hi_offsets_2, hi_offsets_2+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=3;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 2");
    BOOST_CHECK( ndh.h5_write( lowframe[2]) == 0);
    dimsize_t lo_offsets_2[] = {0,2,2};
    test_offsets = vec_ds_t(lo_offsets_2, lo_offsets_2+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=3;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );


    BOOST_TEST_MESSAGE("Closing file");
    BOOST_CHECK( ndh.h5_close() == 0);

}

/** Send 8 NDArray frames, split in a high/low half through the NDArrayToHDF5 class.
 * The frames are sent through sequentially. This test is intended to demonstrate the
 * normal use of the system.
 */
BOOST_AUTO_TEST_CASE(frames_attr_offset_loop)
{
    vec_ds_t test_offsets = vec_ds_t(3,0);
    vec_ds_t test_dset_dims = vec_ds_t(3,0);
    WriteConfig wc;
    add_attr_origin();

    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;
    BOOST_CHECK_NO_THROW( ndh.h5_configure(hiframe[0]));

    BOOST_TEST_MESSAGE("Open file: test_frames_attr_offset_loop.h5");
    BOOST_CHECK( ndh.h5_open("test_frames_attr_offset_loop.h5") == 0);

    test_dset_dims[0]=6; test_dset_dims[1]=4;
    for (int i = 0; i < 8; i++) {
        test_dset_dims[2]=i+1;

        BOOST_TEST_MESSAGE("Writing frame high " << i);
        BOOST_CHECK( ndh.h5_write( hiframe[i]) == 0);
        test_offsets[1]=0; test_offsets[2]=i;
        wc = ndh.get_conf();
        BOOST_CHECK( wc.get_offsets() == test_offsets );
        BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

        BOOST_TEST_MESSAGE("Writing frame low " << i);
        BOOST_CHECK( ndh.h5_write( lowframe[i]) == 0);
        test_offsets[1]=2; test_offsets[2]=i;
        wc = ndh.get_conf();
        BOOST_CHECK( wc.get_offsets() == test_offsets );
        BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );
    }

    BOOST_TEST_MESSAGE("Closing file");
    BOOST_CHECK( ndh.h5_close() == 0);

}

/** Push three frames through in unordered sequence.
 * This is a theoretical scenario within an MPI job where one job may have dropped
 * a frame. This is not intended to be happening as dropped frames should be replaced
 * by an empty frame that increments offset. However, it is useful to know how the
 * system behaves in that situation and this test just demonstrate that.
 */
BOOST_AUTO_TEST_CASE(frames_attr_offset_unordered)
{
    vec_ds_t test_offsets;
    vec_ds_t test_dset_dims = vec_ds_t(3,0);
    WriteConfig wc;
    add_attr_origin();
    test_dset_dims[0]=6; test_dset_dims[1]=4;

    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;
    BOOST_CHECK_NO_THROW( ndh.h5_configure(hiframe[0]));

    BOOST_TEST_MESSAGE("Open file: test_frames_attr_offset_unordered.h5");
    BOOST_CHECK( ndh.h5_open("test_frames_attr_offset_unordered.h5") == 0);

    //-------------  First frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 0");
    BOOST_CHECK( ndh.h5_write( hiframe[0]) == 0);
    dimsize_t hi_offsets_0[] = {0,0,0};
    test_offsets = vec_ds_t(hi_offsets_0, hi_offsets_0+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=1;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 0");
    BOOST_CHECK( ndh.h5_write( lowframe[0]) == 0);
    dimsize_t lo_offsets_0[] = {0,2,0};
    test_offsets = vec_ds_t(lo_offsets_0, lo_offsets_0+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=1;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    //-------------  Third frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 2");
    BOOST_CHECK( ndh.h5_write( hiframe[2]) == 0);
    dimsize_t hi_offsets_2[] = {0,0,2};
    test_offsets = vec_ds_t(hi_offsets_2, hi_offsets_2+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=3;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 2");
    BOOST_CHECK( ndh.h5_write( lowframe[2]) == 0);
    dimsize_t lo_offsets_2[] = {0,2,2};
    test_offsets = vec_ds_t(lo_offsets_2, lo_offsets_2+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=3;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    //-------------  Second frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 1");
    BOOST_CHECK( ndh.h5_write( hiframe[1]) == 0);
    dimsize_t hi_offsets_1[] = {0,0,1};
    test_offsets = vec_ds_t(hi_offsets_1, hi_offsets_1+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=3;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 1");
    BOOST_CHECK( ndh.h5_write( lowframe[1]) == 0);
    dimsize_t lo_offsets_1[] = {0,2,1};
    test_offsets = vec_ds_t(lo_offsets_1, lo_offsets_1+3);
    wc = ndh.get_conf();
    BOOST_CHECK( wc.get_offsets() == test_offsets );
    test_dset_dims[2]=3;
    BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Closing file");
    BOOST_CHECK( ndh.h5_close() == 0);

}

/** Test the automatic offset incrementing when pushing NDArrays through sequentially.
 * Note that the ROI NDArrays in this test are half height of the full dataset as
 * the lower half would need to have some NDAttribute defined to know where it fits in
 * the dataset.
 */
BOOST_AUTO_TEST_CASE(frames_auto_offset_loop)
{
    vec_ds_t test_offsets = vec_ds_t(3,0);
    vec_ds_t test_dset_dims = vec_ds_t(3,0);
    WriteConfig wc;

    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;
    BOOST_CHECK_NO_THROW( ndh.h5_configure(hiframe[0]));

    BOOST_TEST_MESSAGE("Open file: test_frames_auto_offset.h5");
    BOOST_CHECK( ndh.h5_open("test_frames_auto_offset.h5") == 0);

    test_dset_dims[0]=6; test_dset_dims[1]=4;
    for (int i = 0; i < 8; i++) {
        test_dset_dims[2]=i+1;

        BOOST_TEST_MESSAGE("Writing frame high " << i);
        BOOST_CHECK( ndh.h5_write( hiframe[i]) == 0);
        test_offsets[1]=0; test_offsets[2]=i;
        wc = ndh.get_conf();
        BOOST_CHECK( wc.get_offsets() == test_offsets );
        BOOST_CHECK( wc.get_dset_dims() == test_dset_dims );
    }

    BOOST_TEST_MESSAGE("Closing file");
    BOOST_CHECK( ndh.h5_close() == 0);
}

BOOST_AUTO_TEST_SUITE_END()
