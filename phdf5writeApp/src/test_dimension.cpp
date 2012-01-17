/*
 * test_dimension.cpp
 *
 *  Created on: 10 Jan 2012
 *      Author: up45
 *
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DimensionClass
#include <boost/test/unit_test.hpp>
//#include <boost/test/included/unit_test_framework.hpp> // for a static build use this and comment out BOOST_TEST_DYN_LINK

#include <vector>
#include <NDArray.h>
#include "dimension.h"

void util_fill_ndarr_dims(NDArray &ndarr, unsigned long int *sizes, int ndims)
{
    int i=0;
    ndarr.ndims = ndims;
    for (i=0; i<ndims; i++)
        ndarr.initDimension(&(ndarr.dims[i]), sizes[i]);
}

struct DimensionFixture
{
    NDArray ndarr_2d;
    NDArray ndarr_3d;
    unsigned long int sizes[3];
    NDArrayInfo_t ndarr_info;

    DimensionFixture()
    {
        BOOST_TEST_MESSAGE("setup DimensionFixture");
        sizes[0]=2; sizes[1]=4; sizes[2]=6;

        ndarr_2d.dataType = NDUInt16;
        util_fill_ndarr_dims(ndarr_2d, sizes, 2);

        ndarr_3d.dataType = NDUInt16;
        util_fill_ndarr_dims(ndarr_3d, sizes, 3);
    }

    ~DimensionFixture()
    {
        BOOST_TEST_MESSAGE("teardown DimensionFixture");
    }
};

BOOST_FIXTURE_TEST_SUITE(dimension_constructors, DimensionFixture)

BOOST_AUTO_TEST_CASE(simple)
{
    DimensionDesc dim;
    BOOST_CHECK(dim.num_dimensions() == 0);
    BOOST_CHECK(dim.element_size == 0);
    BOOST_CHECK(dim.data_num_elements() == 0);
    BOOST_CHECK(dim.data_num_bytes() == 0);
}

BOOST_AUTO_TEST_CASE(manual_2d)
{
    const int NUM_DIMS=2;
    size_t elementsize = sizeof(int);
    DimensionDesc dim(NUM_DIMS, sizes, elementsize);
    BOOST_CHECK(dim.num_dimensions() == NUM_DIMS);
    BOOST_CHECK(dim.element_size == elementsize);
    BOOST_CHECK(dim.data_num_elements() == sizes[0]*sizes[1]);
    BOOST_CHECK(dim.data_num_bytes() == (elementsize*sizes[0]*sizes[1]) );
}

BOOST_AUTO_TEST_CASE(manual_3d)
{
    const int NUM_DIMS=3;
    size_t elementsize = sizeof(int);
    DimensionDesc dim(NUM_DIMS, sizes, elementsize);
    BOOST_CHECK(dim.num_dimensions() == NUM_DIMS);
    BOOST_CHECK(dim.element_size == elementsize);
    BOOST_CHECK(dim.data_num_elements() == sizes[0]*sizes[1]*sizes[2]);
    BOOST_CHECK(dim.data_num_bytes() == (elementsize*sizes[0]*sizes[1]*sizes[2]) );
}

BOOST_AUTO_TEST_CASE(ndarray_2d)
{
    NDArrayInfo_t ndarr_info;
    ndarr_2d.getInfo(&ndarr_info);
    DimensionDesc dim(ndarr_2d);

    BOOST_CHECK(dim.num_dimensions() == ndarr_2d.ndims);
    BOOST_CHECK(dim.element_size == ndarr_info.bytesPerElement);
    BOOST_CHECK(dim.data_num_elements() ==
            ndarr_2d.dims[0].size * ndarr_2d.dims[1].size);
    BOOST_CHECK(dim.data_num_bytes() ==
            ndarr_info.bytesPerElement * ndarr_2d.dims[0].size * ndarr_2d.dims[1].size);
}

BOOST_AUTO_TEST_CASE(ndarray_3d)
{
    NDArrayInfo_t ndarr_info;
    ndarr_3d.getInfo(&ndarr_info);
    DimensionDesc dim(ndarr_3d);

    BOOST_CHECK(dim.num_dimensions() == ndarr_3d.ndims);
    BOOST_CHECK(dim.element_size == ndarr_info.bytesPerElement);
    BOOST_CHECK(dim.data_num_elements() ==
            ndarr_3d.dims[0].size * ndarr_3d.dims[1].size * ndarr_3d.dims[2].size);
    BOOST_CHECK(dim.data_num_bytes() ==
            ndarr_info.bytesPerElement * ndarr_3d.dims[0].size * ndarr_3d.dims[1].size * ndarr_3d.dims[2].size);
}

BOOST_AUTO_TEST_CASE(copy_3d)
{
    DimensionDesc dim1(ndarr_3d);
    DimensionDesc dim2(dim1);
    BOOST_CHECK(dim1.element_size == dim2.element_size);
    BOOST_CHECK(dim1.num_dimensions() == dim2.num_dimensions());
    BOOST_CHECK(dim1.data_num_elements() == dim2.data_num_elements());

    // Check that the acutal dimension array is not just a shallow copy (pointer copy)
    BOOST_CHECK(dim1.dim_sizes() != dim2.dim_sizes());

}

BOOST_AUTO_TEST_SUITE_END()

//============================================================================
/** Suite to test the features of the DimensionDesc class.
 *
 * Features include assignment operator, public methods.
 */
struct DimensionFeatureFixture {
    NDArray ndarr_small_even;
    NDArray ndarr_small_uneven;
    NDArray ndarr_large_even;
    NDArray ndarr_flat_frame;
    NDArray ndarr_2d_frame;
    NDArray ndarr_6d_dset;
    unsigned long int small_even_sizes[3];
    unsigned long int small_uneven_sizes[3];
    unsigned long int large_even_sizes[3];
    unsigned long int flat_frame_sizes[3];
    unsigned long int frame_2d_sizes[2];
    unsigned long int dset_6d_sizes[6];

    DimensionFeatureFixture(){
        BOOST_TEST_MESSAGE("Setup DimensionFeatureFixture");
        small_even_sizes[0]=2; small_even_sizes[1]=4; small_even_sizes[2]=6;
        ndarr_small_even.dataType = NDUInt16;
        util_fill_ndarr_dims(ndarr_small_even, small_even_sizes, 3);

        small_uneven_sizes[0]=3; small_uneven_sizes[1]=7; small_uneven_sizes[2]=13;
        ndarr_small_uneven.dataType = NDUInt16;
        util_fill_ndarr_dims(ndarr_small_uneven, small_uneven_sizes, 3);

        large_even_sizes[0]=10; large_even_sizes[1]=20; large_even_sizes[2]=20;
        ndarr_large_even.dataType = NDUInt16;
        util_fill_ndarr_dims(ndarr_large_even, large_even_sizes, 3);

        flat_frame_sizes[0]=1; flat_frame_sizes[1]=13; flat_frame_sizes[2]=23;
        ndarr_flat_frame.dataType = NDUInt16;
        util_fill_ndarr_dims(ndarr_flat_frame, flat_frame_sizes, 3);

        frame_2d_sizes[0]=3; frame_2d_sizes[1]=7;
        ndarr_2d_frame.dataType = NDUInt16;
        util_fill_ndarr_dims(ndarr_2d_frame, frame_2d_sizes, 2);

        dset_6d_sizes[0] = 23;
        dset_6d_sizes[1] = 45;
        dset_6d_sizes[2] = 4;
        dset_6d_sizes[3] = 5;
        dset_6d_sizes[4] = 6;
        dset_6d_sizes[5] = 2;
        ndarr_6d_dset.dataType = NDUInt16;
        util_fill_ndarr_dims(ndarr_6d_dset, dset_6d_sizes, 6);
}
    ~DimensionFeatureFixture(){
        BOOST_TEST_MESSAGE("Teardown DimensionFeatureFixture");
    }
};
BOOST_FIXTURE_TEST_SUITE(dimension_features, DimensionFeatureFixture)


BOOST_AUTO_TEST_CASE(assignment)
{
    DimensionDesc dim1(ndarr_small_even);
    DimensionDesc dim2;
    dim2 = dim1;
    BOOST_CHECK(dim1.element_size == dim2.element_size);
    BOOST_CHECK(dim1.num_dimensions() == dim2.num_dimensions());
    BOOST_CHECK(dim1.data_num_elements() == dim2.data_num_elements());

    // Check that the acutal dimension array is not just a shallow copy (pointer copy)
    BOOST_CHECK(dim1.dim_sizes() != dim2.dim_sizes());

    // Check that the individual dimension sizes are equal
    BOOST_CHECK(dim1.dim_sizes()[0] == dim2.dim_sizes()[0]);
    BOOST_CHECK(dim1.dim_sizes()[1] == dim2.dim_sizes()[1]);
    BOOST_CHECK(dim1.dim_sizes()[2] == dim2.dim_sizes()[2]);

}

BOOST_AUTO_TEST_CASE(num_fit_1)
{
    DimensionDesc dim1(ndarr_small_even);
    DimensionDesc dim2(ndarr_large_even);
    BOOST_CHECK( dim1.num_fits(dim2, false) == 5*5*3 ); // fits inside container (no overlap)
    BOOST_CHECK( dim1.num_fits(dim2, true) == 5*5*4 ); // cover full container (with overlap)
}

BOOST_AUTO_TEST_CASE(num_fit_2)
{
    DimensionDesc dim1(ndarr_small_uneven);
    DimensionDesc dim2(ndarr_large_even);
    BOOST_CHECK( dim1.num_fits(dim2, false) == 3*2*1 ); // fits inside container (no overlap)
    BOOST_CHECK( dim1.num_fits(dim2, true) == 4*3*2 ); // cover full container (with overlap)
}

// Boundry case: the container being smaller size than the block
BOOST_AUTO_TEST_CASE(num_fit_3)
{
    DimensionDesc dim1(ndarr_small_uneven);
    DimensionDesc dim2(ndarr_large_even);
    BOOST_CHECK( dim2.num_fits(dim1, false) == 0 ); // fits inside container (no overlap)
    BOOST_CHECK( dim2.num_fits(dim1, true) == 1 ); // cover full container (with overlap)
}

// Boundry case: the block has fewer dimensions than the container
//               This should be OK as the block should expand it's number
//               of dimensions temporarily with size 1 to match the container.
BOOST_AUTO_TEST_CASE(num_fit_4)
{
    DimensionDesc dim1(ndarr_2d_frame);
    DimensionDesc dim2(ndarr_large_even);
    BOOST_CHECK( dim1.num_fits(dim2, false) == 3*2*20 ); // fits inside container (no overlap)
    BOOST_CHECK( dim1.num_fits(dim2, true) == 4*3*20 ); // cover full container (with overlap)
}

// Boundry case: the block has more dimensions than the container
//               This is not ok so the return value is -1
BOOST_AUTO_TEST_CASE(num_fit_5)
{
    DimensionDesc dim1(ndarr_2d_frame);
    DimensionDesc dim2(ndarr_large_even);
    BOOST_CHECK( dim2.num_fits(dim1, false) == -1 ); // fits inside container (no overlap)
    BOOST_CHECK( dim2.num_fits(dim1, true) == -1 ); // cover full container (with overlap)
}

// Boundry case: the block has fewer dimensions than the container
//               This should be OK as the block should expand it's number
//               of dimensions temporarily with size 1 to match the container.
BOOST_AUTO_TEST_CASE(num_fit_6)
{
    DimensionDesc dim1(ndarr_2d_frame);
    DimensionDesc dim2(ndarr_6d_dset);
    BOOST_CHECK( dim1.num_fits(dim2, false) == 7*6*4*5*6*2); // fits inside container (no overlap)
    BOOST_CHECK( dim1.num_fits(dim2, true) == 8*7*4*5*6*2 ); // cover full container (with overlap)
}


BOOST_AUTO_TEST_CASE(object_string)
{
    DimensionDesc dim(ndarr_small_even);
    BOOST_TEST_MESSAGE( dim._str_() );
}

BOOST_AUTO_TEST_CASE(append_dimdesc)
{
    DimensionDesc dim1(ndarr_small_even);
    DimensionDesc dim2(ndarr_small_uneven);
    int numdimsum = dim1.num_dimensions() + dim2.num_dimensions();
    dim1 += dim2;
    BOOST_CHECK( dim1.num_dimensions() == numdimsum);
}

BOOST_AUTO_TEST_CASE(append_dimsize)
{
    DimensionDesc dim1(ndarr_small_even);
    int numdimsum = dim1.num_dimensions() + 1;
    dim1 += 78;
    BOOST_CHECK( dim1.num_dimensions() == numdimsum);
    BOOST_CHECK(dim1.dim_sizes()[numdimsum-1] == 78 );
}

BOOST_AUTO_TEST_CASE(is_equal)
{
    DimensionDesc dim1(ndarr_small_even);
    DimensionDesc dim2(ndarr_small_even);
    DimensionDesc dim3(ndarr_small_uneven);

    BOOST_CHECK( dim1==dim2 );
    BOOST_CHECK( not(dim1==dim3) );
    BOOST_CHECK( dim1!=dim3 );
    BOOST_CHECK( not( dim1!=dim2 ));
}

BOOST_AUTO_TEST_CASE(grow_by_block)
{
    DimensionDesc dim1(ndarr_small_even);
    DimensionDesc dim2(ndarr_large_even);
    DimensionDesc dim3 = dim2;
    BOOST_CHECK( dim3==dim2 );

    BOOST_TEST_MESSAGE( dim1._str_() );
    BOOST_TEST_MESSAGE( dim2._str_() );
    dim2.grow_by_block(dim1);
    BOOST_TEST_MESSAGE( dim2._str_() );
    BOOST_CHECK( dim2 != dim3 );

    int num_elements_mult = dim1.data_num_elements()*dim1.num_fits(dim3,true);
    BOOST_CHECK( num_elements_mult == dim2.data_num_elements() );

}

BOOST_AUTO_TEST_CASE(frame_chunk_cache)
{
    DimensionDesc dim1(ndarr_small_even);
    DimensionDesc dim2(ndarr_flat_frame);
    DimensionDesc dim3 = dim2;
    BOOST_CHECK( dim3==dim2 );

    BOOST_TEST_MESSAGE( dim1._str_() );
    BOOST_TEST_MESSAGE( dim2._str_() );
    dim2.grow_by_block(dim1);
    BOOST_TEST_MESSAGE( dim2._str_() );
    BOOST_CHECK( dim2 != dim3 );

    int num_elements_mult = dim1.data_num_elements()*dim1.num_fits(dim3,true);
    BOOST_CHECK( num_elements_mult == dim2.data_num_elements() );

}


BOOST_AUTO_TEST_SUITE_END()
