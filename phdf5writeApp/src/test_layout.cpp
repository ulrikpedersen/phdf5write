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

//#include <vector>
//#include <NDArray.h>
#include "layout.h"

BOOST_AUTO_TEST_SUITE( constructors )

BOOST_AUTO_TEST_CASE(hdf_element)
{
    HdfElement *hdf = new HdfElement();
    BOOST_CHECK_EQUAL( hdf->get_name(), "entry");
    delete hdf;
    BOOST_CHECK(true);

    HdfElement h("somename");
    BOOST_CHECK_EQUAL(h.get_name(), "somename" );
    HdfElement g;
    BOOST_CHECK_NO_THROW( g = h );
    BOOST_CHECK_EQUAL(h.get_name(), g.get_name() );

    HdfElement i(g);
    BOOST_CHECK_EQUAL(i.get_name(), g.get_name() );
    BOOST_CHECK_EQUAL(i.get_full_name(), "/somename");
}

BOOST_AUTO_TEST_CASE(hdf_group)
{
    const char *name="mygroup";
    HdfGroup *grp = new HdfGroup(name);
    BOOST_CHECK_EQUAL( grp->get_name(), name);
    BOOST_CHECK_EQUAL( grp->num_groups(), 0);
    delete grp;
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(hdf_group_noptr)
{
    const char *name="mygroup";
    HdfGroup grp(name);
    BOOST_CHECK_EQUAL( grp.get_name(), name);
    BOOST_CHECK_EQUAL( grp.num_groups(), 0);
    BOOST_CHECK(true);
}


BOOST_AUTO_TEST_CASE(hdf_group_2)
{
    const char *name="grp1";
    HdfGroup *grp = new HdfGroup(name);
    BOOST_CHECK_EQUAL( grp->get_name(), name);
    BOOST_CHECK_EQUAL( grp->num_groups(), 0);

    std::string name1("grp1_1");
    HdfGroup *grp1 = NULL;
    BOOST_TEST_MESSAGE( "grp ptr: " << grp << " ngrp ptr: " << grp1);
    grp1 = grp->new_group(name1);
    BOOST_CHECK( grp1 != NULL);
    BOOST_TEST_MESSAGE( "grp ptr: " << grp << " ngrp ptr: " << grp1);
    BOOST_CHECK_EQUAL( grp->num_groups(), 1);

    std::string name2("grp1_2");
    HdfGroup *grp2 =NULL;
    BOOST_CHECK_NO_THROW(grp2=grp->new_group(name2));
    BOOST_CHECK( grp2 != NULL);
    BOOST_CHECK_EQUAL( grp->num_groups(), 2);

    std::string name3( "grp1_1_1");
    HdfGroup *grp3 = NULL;
    BOOST_CHECK_NO_THROW( grp3 = grp1->new_group(name3) );
    BOOST_CHECK( grp3 != NULL);
    BOOST_CHECK_EQUAL( grp1->num_groups(), 1);
    BOOST_CHECK_EQUAL( grp->num_groups(), 2);

    std::string name4("grp1_3");
    BOOST_CHECK_NO_THROW(grp->new_group(name4));
    BOOST_CHECK_EQUAL( grp->num_groups(), 3);

    // Delete the whole lot
    BOOST_CHECK(true);
    BOOST_CHECK_NO_THROW( delete grp );
}


BOOST_AUTO_TEST_CASE(hdf_group_2_noptr)
{
    const char *name="grp1";
    HdfGroup grp(name);
    BOOST_CHECK_EQUAL( grp.get_name(), name);
    BOOST_CHECK_EQUAL( grp.num_groups(), 0);

    std::string  name1("grp1_1");
    HdfGroup grp1;
    BOOST_CHECK_EQUAL( grp1.num_groups(), 0);
    BOOST_CHECK_NO_THROW( grp1 = *(grp.new_group(name1)) );
    BOOST_CHECK_EQUAL( grp1.get_name(), name1);
    BOOST_CHECK_EQUAL( grp.num_groups(), 1);

    std::string  name2 ("grp1_1_1");
    HdfGroup grp2;
    BOOST_CHECK_NO_THROW(grp2 = *(grp.new_group(name2)));
    BOOST_CHECK_EQUAL( grp2.get_name(), name2);
    BOOST_CHECK_EQUAL( grp1.num_groups(), 0);
    BOOST_CHECK_EQUAL( grp2.num_groups(), 0);
    BOOST_CHECK_EQUAL( grp.num_groups(), 2);

    BOOST_CHECK(true);
    // Reminder: destructors called on grp1 and grp3 as they go out of scope
}

BOOST_AUTO_TEST_CASE(hdf_group_2_mix_ptr)
{
    const char *name="grp1";
    HdfGroup grp(name);
    BOOST_CHECK_EQUAL( grp.get_name(), name);
    BOOST_CHECK_EQUAL( grp.num_groups(), 0);

    const char * name1 = "grp1_1";
    std::string name1_str(name1);
    BOOST_CHECK_NE( (long int)grp.new_group(name1_str), 0);
    BOOST_CHECK_EQUAL( grp.num_groups(), 1);

    const char * name2 = "grp1_1_1";
    std::string name2_str(name2);
    BOOST_CHECK_NE( (long int)grp.new_group(name2_str), 0);
    BOOST_CHECK_EQUAL( grp.num_groups(), 2);

    BOOST_CHECK(true);
    // Reminder: destructors called on grp1 and grp3 as they go out of scope
}


BOOST_AUTO_TEST_CASE(hdf_dataset)
{
    const char* name="mydset";
    HdfDataset *dset = new HdfDataset(name);
    BOOST_CHECK_EQUAL( dset->get_name(), name);
    delete dset;
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(insert_dset_in_group)
{
    HdfGroup *grp = new HdfGroup();
    BOOST_CHECK_EQUAL( grp->num_datasets(), 0);

    std::string dsetname ("mydset");
    HdfDataset *dset = NULL;
    BOOST_CHECK_NO_THROW(dset = grp->new_dset( dsetname ));
    BOOST_CHECK_NE((long int)dset, 0);
    BOOST_CHECK_EQUAL( dset->get_name(), dsetname);
    BOOST_CHECK_EQUAL(grp->num_datasets(), 1);
    BOOST_CHECK_EQUAL(grp->num_groups(), 0);

    const char *dsetname2 = "dset2";
    std::string dsetname2_str(dsetname2);
    BOOST_CHECK( grp->new_dset(dsetname2_str) != NULL);
    BOOST_CHECK_EQUAL(grp->num_datasets(), 2);
    BOOST_CHECK_EQUAL(grp->num_groups(), 0);

    BOOST_CHECK(true);
    delete grp;
}

BOOST_AUTO_TEST_CASE(insert_dset_in_group_2)
{
    HdfGroup grp;
    BOOST_CHECK_EQUAL( grp.num_datasets(), 0);

    const char *dsetname = "mydset";
    HdfDataset dset(dsetname);
    std::string dsetname_str = dset.get_name();
    BOOST_CHECK_EQUAL( dsetname_str, dsetname);

    BOOST_CHECK(grp.new_dset( dsetname_str ) != NULL);
    BOOST_CHECK_EQUAL(grp.num_datasets(), 1);
    BOOST_CHECK_EQUAL(grp.num_groups(), 0);

    const char *dsetname2 = "dset2";
    std::string dsetname2_str(dsetname2);
    BOOST_CHECK( grp.new_dset(dsetname2_str) !=NULL);
    BOOST_CHECK_EQUAL(grp.num_datasets(), 2);
    BOOST_CHECK_EQUAL(grp.num_groups(), 0);

    BOOST_CHECK(true);
}



BOOST_AUTO_TEST_SUITE_END()


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
    BOOST_REQUIRE_EQUAL(root.get_name(), "entry");
    BOOST_REQUIRE_EQUAL(root.get_path(), "/");

    HdfGroup *inst_ptr = NULL;
    BOOST_REQUIRE_NO_THROW(inst_ptr=root.new_group( "instrument" ));
    BOOST_REQUIRE( inst_ptr != NULL );
    BOOST_REQUIRE_EQUAL( inst_ptr->get_path(false), "/entry");
    BOOST_REQUIRE_EQUAL( inst_ptr->get_path(true), "/entry/");
    BOOST_REQUIRE_EQUAL( inst_ptr->get_full_name(), "/entry/instrument");
    BOOST_REQUIRE_EQUAL( inst_ptr->tree_level(), 2);


    std::string dsetname = "mydata";
    // Create and insert a new dataset by name, get the name back and
    // compare it with the input name.
    BOOST_REQUIRE_NO_THROW( inst_ptr->new_dset( dsetname ) );
    HdfDataset *dset;
    BOOST_REQUIRE_EQUAL( inst_ptr->find_dset(dsetname, &dset), 0);
    BOOST_REQUIRE_EQUAL( dset->get_name(), dsetname);

    std::string dsetfullname = "/entry/instrument/mydata";
    std::string fn = dset->get_full_name();
    BOOST_REQUIRE_EQUAL( fn, dsetfullname);
    BOOST_REQUIRE_EQUAL( dset->tree_level(), 3);
    BOOST_CHECK( true ); // check whether destructors work
}

BOOST_AUTO_TEST_CASE( retrieve_modify_dset )
{
    HdfGroup root("root");
    BOOST_REQUIRE_EQUAL(root.num_datasets(), 0);
    BOOST_REQUIRE_EQUAL( root.num_groups(), 0);

    // create and insert a dataset
    HdfDataset *dset_ptr = NULL;
    BOOST_REQUIRE_NO_THROW( dset_ptr = root.new_dset("mydset") );
    BOOST_REQUIRE( dset_ptr != NULL );

    // Create a dset instance to be filled in by reference
    HdfDataset *dset;
    std::string dset_str("mydset");
    BOOST_REQUIRE_EQUAL( root.find_dset(dset_str, &dset), 0);
    BOOST_REQUIRE_EQUAL( dset->get_name(), dset_str);
    BOOST_REQUIRE_EQUAL( dset->get_name(), dset_ptr->get_name());

    std::string attr_name("myattr");
    HdfAttribute attr(attr_name);
    BOOST_REQUIRE_EQUAL( dset->add_attribute(attr), 0);

    BOOST_REQUIRE( dset->has_attribute(attr_name));
    BOOST_REQUIRE( dset_ptr->has_attribute(attr_name));

    dset = NULL;
    BOOST_REQUIRE_EQUAL( root.find_dset_ndattr(attr_name, &dset), 0);
    BOOST_CHECK( dset != NULL );
}

BOOST_AUTO_TEST_CASE( retrieve_dset_tree )
{
    HdfGroup root("root");
    BOOST_REQUIRE_EQUAL(root.num_datasets(), 0);
    BOOST_REQUIRE_EQUAL( root.num_groups(), 0);

    HdfGroup *grp1 = NULL;
    BOOST_REQUIRE_NO_THROW( grp1 = root.new_group("group1") );
    BOOST_CHECK( grp1 != NULL );
    BOOST_REQUIRE_NO_THROW( grp1->new_dset("blah1")); // insert some dummy datasets
    BOOST_REQUIRE_NO_THROW( grp1->new_dset("nnnn1"));

    HdfGroup *grp2 = NULL;
    BOOST_REQUIRE_NO_THROW( grp2 = grp1->new_group("group2") );
    BOOST_REQUIRE( grp2 != NULL );
    BOOST_REQUIRE_NO_THROW( grp2->new_dset("blah2"));
    BOOST_REQUIRE_NO_THROW( grp2->new_dset("bbbb2"));

    // Create a dset instance to be filled in by reference
    HdfDataset *dset = NULL;
    std::string dset_str("mydset");
    BOOST_REQUIRE_NO_THROW( dset=grp2->new_dset(dset_str) );
    BOOST_CHECK( dset != NULL );

    //  Create an attribute and attach to the dataset
    std::string attr_name("myattr");
    HdfAttribute attr(attr_name);
    BOOST_REQUIRE_EQUAL( dset->add_attribute(attr), 0);

    // Finally search the root for the dataset with the named attribute
    HdfDataset *result = NULL;
    BOOST_REQUIRE_EQUAL( root.find_dset_ndattr(attr_name, &result), 0);
    // and check that the dataset is the very same instance
    BOOST_REQUIRE_EQUAL( (long int)result, (long int)dset);
    BOOST_REQUIRE_EQUAL( result->get_full_name(), dset->get_full_name());
    BOOST_TEST_MESSAGE( "root: " << root._str_() );
    BOOST_TEST_MESSAGE( "dset: " << dset->_str_() );
}

BOOST_AUTO_TEST_CASE( hdfdataset_not_present )
{
    HdfGroup root("root");

    // Search for a non-existing dataset
    std::string dset_str("neverland");
    HdfDataset *dset_ptr = NULL;
    BOOST_REQUIRE_NE( root.find_dset(dset_str, &dset_ptr), 0);
    BOOST_REQUIRE( dset_ptr == NULL );

    dset_ptr = NULL;
    BOOST_REQUIRE_NE( root.find_dset_ndattr(dset_str, &dset_ptr), 0);
    BOOST_REQUIRE( dset_ptr == NULL );
}

BOOST_AUTO_TEST_SUITE_END()

struct hdf_tree_fixture {
	HdfGroup entry;
	HdfGroup *instrument;
	HdfGroup *detector;
	HdfDataset *data;
	HdfDataset *ndattribute_one;
	HdfDataset *ndattribute_two;

	hdf_tree_fixture(){
        BOOST_TEST_MESSAGE("setup hdf_tree_fixture");
        entry = HdfGroup("entry");
        instrument = entry.new_group("instrument");
        detector = entry.new_group("detector");

        // Dataset 'data' with one hdf string attribute: signal="1"
        data = detector->new_dset("data");
        HdfAttribute hdfattr("signal");
        std::string signal_value("1");
        hdfattr.source = HdfDataSource(phdf_constant, signal_value);
        data->add_attribute(hdfattr);
        HdfDataSource data_src(phdf_detector, phdf_uint32);
        data->set_data_source(data_src); // The source of 'data' is a detector.

        // instrument group is the default group for NDAttributes
        instrument->set_default_ndattr_group();

        // NDAttribute datasets
        data_src = HdfDataSource(phdf_ndattribute, phdf_uint8);
        ndattribute_one = instrument->new_dset("one");
        ndattribute_one->set_data_source(data_src);
        ndattribute_two = instrument->new_dset("two");
        ndattribute_two->set_data_source(data_src);

    }
    ~hdf_tree_fixture(){
        BOOST_TEST_MESSAGE("teardown hdf_tree_fixture");
    }
};

BOOST_FIXTURE_TEST_SUITE(hdf_tree, hdf_tree_fixture)

BOOST_AUTO_TEST_CASE(ndattr_default)
{
	// Set the instrument group to be the default container of NDAttributes
	BOOST_REQUIRE_NO_THROW( instrument->set_default_ndattr_group() );
	// Check that the instrument group instance get returned as the default
	// container of NDAttributes
	BOOST_REQUIRE_EQUAL( entry.find_ndattr_default_group(), instrument );
}

BOOST_AUTO_TEST_CASE(datatype)
{
	BOOST_TEST_MESSAGE("Root tree: " << entry._str_());
	// Check that the dataset 'data' has an hdf attribute called 'signal'
	std::string str_signal("signal");
	BOOST_REQUIRE( data->has_attribute(str_signal));

	// Check that we can search for and find the dataset 'data', based on it's
	// hdf attribute 'signal'
	HdfDataset *dset;
	BOOST_REQUIRE_EQUAL( entry.find_dset_ndattr("signal", &dset), 0);
	BOOST_REQUIRE_EQUAL( dset, data);

	// Check that 'data' comes from a detector and has the right datatype: phdf_uint32
	BOOST_REQUIRE_EQUAL( data->data_source().is_src_detector(), true );
	BOOST_REQUIRE_EQUAL( data->data_source().get_datatype(), phdf_uint32 );


	// Search from the root of the tree ('entry') for a default NDAttribute
	// container. Then check that the result is indeed the 'instrument' group.
	HdfGroup* def_ndattr_grp;
	BOOST_REQUIRE_NO_THROW( def_ndattr_grp = entry.find_ndattr_default_group() );
	BOOST_REQUIRE( def_ndattr_grp != NULL);
	BOOST_REQUIRE_EQUAL( def_ndattr_grp, instrument );
}

BOOST_AUTO_TEST_SUITE_END()
