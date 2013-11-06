/** \cond
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

using namespace phdf5;

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
#define NFRAMES 8
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
    NDArray hiframe[NFRAMES];
    NDArray lowframe[NFRAMES];
    unsigned long int sizes[NFRAMES], chunks[NFRAMES], dsetdims[NFRAMES];
    unsigned long int lowoffset, hioffset, zero;
    int numframes;
    int fill_value;

    FrameSetFixture()
    {
        BOOST_TEST_MESSAGE("setup DynamicFixture");
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
        BOOST_TEST_MESSAGE("teardown DynamicFixture");
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
    BOOST_REQUIRE_NO_THROW( ndh.h5_configure(hiframe[0]));
    BOOST_REQUIRE_NO_THROW( wc = ndh.get_conf() );
    BOOST_TEST_MESSAGE( "WriteConfig: " << wc._str_() );

    BOOST_TEST_MESSAGE("Open file: test_frames_attr_offset_loop.h5");
    BOOST_REQUIRE_EQUAL( ndh.h5_open("test_frames_attr_offset_loop.h5"), 0);

    test_dset_dims[DIMY]=YSIZE; test_dset_dims[DIMX]=XSIZE;
    for (int i = 0; i < NFRAMES; i++) {
        test_dset_dims[DIMFRAME]=i+1;

        BOOST_TEST_MESSAGE("Writing frame high " << i);
        BOOST_REQUIRE_EQUAL( ndh.h5_write( hiframe[i]), 0);
        test_offsets[1]=0; test_offsets[DIMFRAME]=i;
        BOOST_REQUIRE_NO_THROW( wc = ndh.get_conf() );
        BOOST_TEST_MESSAGE( "WriteConfig: " << wc._str_() );
        BOOST_REQUIRE( wc.get_offsets() == test_offsets );
        BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

        BOOST_TEST_MESSAGE("Writing frame low " << i);
        BOOST_REQUIRE_EQUAL( ndh.h5_write( lowframe[i]), 0);
        test_offsets[1]=2; test_offsets[DIMFRAME]=i;
        BOOST_REQUIRE_NO_THROW( wc = ndh.get_conf() );
        BOOST_REQUIRE( wc.get_offsets() == test_offsets );
        BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );
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
    test_dset_dims[DIMX]=XSIZE; test_dset_dims[DIMY]=YSIZE;

    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;

    BOOST_REQUIRE_NO_THROW( ndh.h5_configure(lowframe[0]) );

    BOOST_TEST_MESSAGE("Open file: test_frames_attr_offset.h5");
    BOOST_REQUIRE_EQUAL( ndh.h5_open("test_frames_attr_offset.h5"), 0);

    //-------------  First frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 0");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( hiframe[0]), 0);
    dimsize_t hi_offsets_0[] = {0,0,0};
    test_offsets = vec_ds_t(hi_offsets_0, hi_offsets_0+3);
    BOOST_REQUIRE_NO_THROW( wc = ndh.get_conf() );
    BOOST_TEST_MESSAGE( "WriteConfig: " << wc._str_() );
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=1;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 0");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( lowframe[0]), 0);
    dimsize_t lo_offsets_0[] = {0,2,0};
    test_offsets = vec_ds_t(lo_offsets_0, lo_offsets_0+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=1;
    BOOST_REQUIRE( wc.get_dset_dims() ==  test_dset_dims );

    //-------------  Second frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 1");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( hiframe[1]), 0);
    dimsize_t hi_offsets_1[] = {1,0,0};
    test_offsets = vec_ds_t(hi_offsets_1, hi_offsets_1+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=2;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 1");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( lowframe[1]), 0);
    dimsize_t lo_offsets_1[] = {1,2,0};
    test_offsets = vec_ds_t(lo_offsets_1, lo_offsets_1+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=2;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    //-------------  Third frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 2");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( hiframe[2]), 0);
    dimsize_t hi_offsets_2[] = {2,0,0};
    test_offsets = vec_ds_t(hi_offsets_2, hi_offsets_2+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=3;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 2");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( lowframe[2]), 0);
    dimsize_t lo_offsets_2[] = {2,2,0};
    test_offsets = vec_ds_t(lo_offsets_2, lo_offsets_2+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=3;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );


    BOOST_TEST_MESSAGE("Closing file");
    BOOST_REQUIRE_EQUAL( ndh.h5_close(), 0);

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
    BOOST_REQUIRE_EQUAL( ndh.h5_open("test_frames_attr_offset_loop.h5"), 0);

    test_dset_dims[DIMY]=YSIZE; test_dset_dims[DIMX]=XSIZE;
    for (int i = 0; i < NFRAMES; i++) {
        test_dset_dims[DIMFRAME]=i+1;

        BOOST_TEST_MESSAGE("Writing frame high " << i);
        BOOST_REQUIRE_EQUAL( ndh.h5_write( hiframe[i]), 0);
        test_offsets[DIMY]=0; test_offsets[DIMFRAME]=i;
        wc = ndh.get_conf();
        BOOST_REQUIRE( wc.get_offsets() == test_offsets );
        BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

        BOOST_TEST_MESSAGE("Writing frame low " << i);
        BOOST_REQUIRE_EQUAL( ndh.h5_write( lowframe[i]), 0);
        test_offsets[DIMY]=2; test_offsets[DIMFRAME]=i;
        wc = ndh.get_conf();
        BOOST_REQUIRE( wc.get_offsets() == test_offsets );
        BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );
    }

    BOOST_TEST_MESSAGE("Closing file");
    BOOST_REQUIRE_EQUAL( ndh.h5_close(), 0);

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
    test_dset_dims[DIMX]=XSIZE; test_dset_dims[DIMY]=YSIZE;

    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;
    BOOST_REQUIRE_NO_THROW( ndh.h5_configure(hiframe[0]));

    BOOST_TEST_MESSAGE("Open file: test_frames_attr_offset_unordered.h5");
    BOOST_REQUIRE_EQUAL( ndh.h5_open("test_frames_attr_offset_unordered.h5"), 0);

    //-------------  First frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 0");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( hiframe[0]), 0);
    dimsize_t hi_offsets_0[] = {0,0,0};
    test_offsets = vec_ds_t(hi_offsets_0, hi_offsets_0+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=1;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 0");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( lowframe[0]), 0);
    dimsize_t lo_offsets_0[] = {0,2,0};
    test_offsets = vec_ds_t(lo_offsets_0, lo_offsets_0+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=1;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    //-------------  Third frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 2");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( hiframe[2]), 0);
    dimsize_t hi_offsets_2[] = {2,0,0};
    test_offsets = vec_ds_t(hi_offsets_2, hi_offsets_2+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=3;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 2");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( lowframe[2]) , 0);
    dimsize_t lo_offsets_2[] = {2,2,0};
    test_offsets = vec_ds_t(lo_offsets_2, lo_offsets_2+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=3;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    //-------------  Second frame -------------------------------
    BOOST_TEST_MESSAGE("Writing frame high 1");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( hiframe[1]) , 0);
    dimsize_t hi_offsets_1[] = {1,0,0};
    test_offsets = vec_ds_t(hi_offsets_1, hi_offsets_1+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=3;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Writing frame low 1");
    BOOST_REQUIRE_EQUAL( ndh.h5_write( lowframe[1]), 0);
    dimsize_t lo_offsets_1[] = {1,2,0};
    test_offsets = vec_ds_t(lo_offsets_1, lo_offsets_1+3);
    wc = ndh.get_conf();
    BOOST_REQUIRE( wc.get_offsets() == test_offsets );
    test_dset_dims[DIMFRAME]=3;
    BOOST_REQUIRE( wc.get_dset_dims() == test_dset_dims );

    BOOST_TEST_MESSAGE("Closing file");
    BOOST_REQUIRE_EQUAL( ndh.h5_close(), 0);

}

/** Test the automatic offset incrementing when pushing NDArrays through sequentially.
 * Note that the ROI NDArrays in this test are half height of the full dataset as
 * the lower half would need to have some NDAttribute defined to know where it fits in
 * the dataset.
 */
BOOST_AUTO_TEST_CASE(frames_auto_offset_loop)
{
    //vec_ds_t test_offsets = vec_ds_t(3,0);
    //vec_ds_t test_dset_dims = vec_ds_t(3,0);
    WriteConfig wc;

    BOOST_TEST_MESSAGE("Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;
    BOOST_REQUIRE_NO_THROW( ndh.h5_configure(hiframe[0]));

    BOOST_TEST_MESSAGE("Open file: test_frames_auto_offset.h5");
    BOOST_REQUIRE_EQUAL( ndh.h5_open("test_frames_auto_offset.h5"), 0);

    //test_dset_dims[DIMX]=XSIZE; test_dset_dims[DIMY]=YSIZE;
    for (int i = 0; i < NFRAMES; i++) {
        //test_dset_dims[DIMFRAME]=i+1;

        BOOST_TEST_MESSAGE("Writing frame high " << i);
        BOOST_REQUIRE_EQUAL( ndh.h5_write( hiframe[i]), 0);
        //test_offsets[DIMFRAME]=i;
        wc = ndh.get_conf();
        BOOST_REQUIRE_EQUAL( wc.get_offsets()[DIMFRAME], (long long unsigned int)i );
        BOOST_REQUIRE_EQUAL( wc.get_dset_dims()[DIMFRAME], (long long unsigned int)i+1 );
    }

    BOOST_TEST_MESSAGE("Closing file");
    BOOST_REQUIRE_EQUAL( ndh.h5_close(), 0);
}

BOOST_AUTO_TEST_CASE(datatype_conversion)
{
	PHDF_DataType_t phdf_datatype = phdf_int32;
	NDDataType_t ndarr_datatype = NDInt16;
	const NDArrayToHDF5 obj;

	BOOST_CHECK_EQUAL(obj.from_ndarr_to_phdf_datatype(NDInt16), phdf_int16);
	BOOST_CHECK_EQUAL(obj.from_ndarr_to_phdf_datatype(NDInt32), phdf_int32);
	BOOST_CHECK_EQUAL(obj.from_ndarr_to_phdf_datatype(NDUInt16), phdf_uint16);
	BOOST_CHECK_EQUAL(obj.from_ndarr_to_phdf_datatype(NDUInt32), phdf_uint32);
	BOOST_CHECK_EQUAL(obj.from_ndarr_to_phdf_datatype(NDFloat64), phdf_float64);
	BOOST_CHECK_EQUAL(obj.from_ndarr_to_phdf_datatype(NDUInt8), phdf_uint8);

	BOOST_CHECK_EQUAL(obj.from_ndattr_to_phdf_datatype(NDAttrUInt8), phdf_uint8);
	BOOST_CHECK_EQUAL(obj.from_ndattr_to_phdf_datatype(NDAttrInt8), phdf_int8);
	BOOST_CHECK_EQUAL(obj.from_ndattr_to_phdf_datatype(NDAttrUInt16), phdf_uint16);
	BOOST_CHECK_EQUAL(obj.from_ndattr_to_phdf_datatype(NDAttrFloat32), phdf_float32);
	BOOST_CHECK_EQUAL(obj.from_ndattr_to_phdf_datatype(NDAttrString), phdf_string);

}

BOOST_AUTO_TEST_SUITE_END()
/** \endcond */
