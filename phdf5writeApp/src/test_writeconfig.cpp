/*
 * test_dimension.cpp
 *
 *  Created on: 10 Jan 2012
 *      Author: up45
 *
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE WriteConfigClass
#include <boost/test/unit_test.hpp>
//#include <boost/test/included/unit_test_framework.hpp> // for a static build use this and comment out BOOST_TEST_DYN_LINK

#include <vector>
#include <string>
#include <NDArray.h>
#include "writeconfig.hpp"

void util_fill_ndarr_dims(NDArray &ndarr, unsigned long int *sizes, int ndims)
{
    int i=0;
    ndarr.ndims = ndims;
    for (i=0; i<ndims; i++)
        ndarr.initDimension(&(ndarr.dims[i]), sizes[i]);
}

//============================================================================
/** Suite to test the features of the DimensionDesc class.
 *
 * Features include assignment operator, public methods.
 */

struct DefaultFixture
{
    NDArray ndarr_2d;
    NDArray ndarr_3d;
    unsigned long int sizes[3];
    unsigned long int chunks[3];
    NDArrayInfo_t ndarr_info;
    DimensionDesc dim_2d;
    DimensionDesc dim_3d;
    std::string fname;
    int intfillvalue;

    DefaultFixture()
    {
        BOOST_TEST_MESSAGE("setup DefaultFixture");
        fname="fname.h5";
        sizes[0]=2; sizes[1]=4; sizes[2]=6;
        chunks[0]=1; chunks[1]=2; chunks[2]=3;

        ndarr_2d.dataType = NDUInt16;
        util_fill_ndarr_dims(ndarr_2d, sizes, 2);

        ndarr_3d.dataType = NDUInt16;
        util_fill_ndarr_dims(ndarr_3d, sizes, 3);

        dim_2d = DimensionDesc(ndarr_2d);
        dim_3d = DimensionDesc(ndarr_3d);

        ndarr_2d.pAttributeList->add("h5_chunk_size_0", "dimension 0", NDAttrUInt32, (void*)(chunks) );
        ndarr_2d.pAttributeList->add("h5_chunk_size_1", "dimension 1", NDAttrUInt32, (void*)(chunks+1) );
        ndarr_2d.pAttributeList->add("h5_chunk_size_2", "dimension 2", NDAttrUInt32, (void*)(chunks+2) );

        intfillvalue = 23;
        ndarr_2d.pAttributeList->add("h5_fill_val", "fill value integer", NDAttrInt32, (void*)&intfillvalue);
        ndarr_2d.pAttributeList->report(11);
    }

    ~DefaultFixture()
    {
        BOOST_TEST_MESSAGE("teardown DefaultFixture");
    }

};

BOOST_FIXTURE_TEST_SUITE(constructors, DefaultFixture)

BOOST_AUTO_TEST_CASE(simple)
{
    int fillvalue = 0;
    void *ptrfillvalue = (void*)&fillvalue;
    size_t nbytes = sizeof(int);

    WriteConfig wc(fname, ndarr_2d);
    BOOST_CHECK( wc.file_name() == fname );
    BOOST_CHECK( wc.alignment == H5_DEFAULT_ALIGN );

    wc.get_fill_value( ptrfillvalue, &nbytes);
    BOOST_CHECK( fillvalue == intfillvalue );

    HSIZE_T cachebytes=0;
    DimensionDesc cache_block = wc.min_chunk_cache();
    cachebytes = cache_block.data_num_bytes();
    BOOST_TEST_MESSAGE( cache_block._str_() );
}

BOOST_AUTO_TEST_SUITE_END()

