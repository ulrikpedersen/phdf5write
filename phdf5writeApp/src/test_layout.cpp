/*
 * test_layout.cpp
 *
 *  Created on: 24 Jan 2012
 *      Author: up45
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LayoutModule
#include <boost/test/unit_test.hpp>
//#include <boost/test/included/unit_test_framework.hpp> // for a static build use this and comment out BOOST_TEST_DYN_LINK

#include <vector>
#include <NDArray.h>
#include "layout.h"

struct hdf_element_fixture {
    hdf_element_fixture(){
        BOOST_TEST_MESSAGE("setup hdf_element_fixture");
    }
    ~hdf_element_fixture(){
        BOOST_TEST_MESSAGE("teardown hdf_element_fixture");
    }
};

BOOST_FIXTURE_TEST_SUITE(hdf_element, hdf_element_fixture)

BOOST_AUTO_TEST_CASE(simple)
{
    HdfGroup root;
    BOOST_CHECK(root.get_name().compare("entry") == 0);

    HdfGroup instrument("instrument");
    BOOST_CHECK(instrument.get_name().compare("instrument") == 0);
    BOOST_CHECK( root.insert_group( &instrument ) == 0 );
    BOOST_CHECK( instrument.get_full_name().compare("/entry/instrument") == 0);
    BOOST_TEST_MESSAGE( "instrument full name: " << instrument.get_full_name() );

    std::string dsetname = "mydata";
    // Create and insert a new dataset by name, get the name back and
    // compare it with the input name.
    BOOST_CHECK( instrument.new_dset( dsetname ) == 0 );
    HdfDataset dset;
    BOOST_CHECK( instrument.find_dset(dsetname, dset) == 0);
    BOOST_CHECK( dset.get_name().compare(dsetname.c_str()) == 0);
    std::string dsetfullname = "/entry/instrument/";
    dsetfullname += dsetname;
    BOOST_TEST_MESSAGE( "Checking dsetfullname is: " << dsetfullname );
    BOOST_TEST_MESSAGE( "dset fullname: " << dset.get_full_name() );
    std::string fn = dset.get_full_name();
    BOOST_TEST_MESSAGE( "fn: " << fn << "dsetfullname: " << dsetfullname );
    BOOST_CHECK( fn.compare(dsetfullname.c_str()) == 0);
    BOOST_CHECK( true ); // check whether destructors work
}

BOOST_AUTO_TEST_SUITE_END()
