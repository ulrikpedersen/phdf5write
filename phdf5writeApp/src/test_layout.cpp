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
    delete grp;
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
    BOOST_CHECK_NE( (int)grp.new_group(name1_str), 0);
    BOOST_CHECK_EQUAL( grp.num_groups(), 1);

    const char * name2 = "grp1_1_1";
    std::string name2_str(name2);
    BOOST_CHECK_NE( (int)grp.new_group(name2_str), 0);
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
    BOOST_CHECK_NE((int)dset, 0);
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
    BOOST_CHECK(root.get_name().compare("entry") == 0);

    HdfGroup *inst_ptr = NULL;
    BOOST_CHECK_NO_THROW(inst_ptr=root.new_group( "instrument" ));
    BOOST_CHECK( inst_ptr!=NULL );
    BOOST_CHECK_EQUAL( inst_ptr->get_full_name(), "/entry/instrument");

    std::string dsetname = "mydata";
    // Create and insert a new dataset by name, get the name back and
    // compare it with the input name.
    BOOST_CHECK_NO_THROW( inst_ptr->new_dset( dsetname ) );
    HdfDataset *dset;
    BOOST_CHECK( inst_ptr->find_dset(dsetname, &dset) == 0);
    BOOST_CHECK_EQUAL( dset->get_name(), dsetname);

    std::string dsetfullname = "/entry/instrument/mydata";
    std::string fn = dset->get_full_name();
    BOOST_CHECK_EQUAL( fn, dsetfullname);
    BOOST_CHECK( true ); // check whether destructors work
}

BOOST_AUTO_TEST_CASE( retrieve_modify_dset )
{
    HdfGroup root("root");
    BOOST_CHECK_EQUAL(root.num_datasets(), 0);
    BOOST_CHECK_EQUAL( root.num_groups(), 0);

    // create and insert a dataset
    HdfDataset *dset_ptr = NULL;
    BOOST_CHECK_NO_THROW( dset_ptr = root.new_dset("mydset") );
    BOOST_CHECK( dset_ptr != NULL );

    // Create a dset instance to be filled in by reference
    HdfDataset *dset;
    std::string dset_str("mydset");
    BOOST_CHECK_EQUAL( root.find_dset(dset_str, &dset), 0);
    BOOST_CHECK_EQUAL( dset->get_name(), dset_str);
    BOOST_CHECK_EQUAL( dset->get_name(), dset_ptr->get_name());

    std::string attr_name("myattr");
    HdfAttribute attr(attr_name);
    BOOST_CHECK_EQUAL( dset->add_attribute(attr), 0);

    BOOST_CHECK( dset->has_attribute(attr_name));
    BOOST_CHECK( dset_ptr->has_attribute(attr_name));

    dset = NULL;
    BOOST_CHECK_EQUAL( root.find_dset_ndattr(attr_name, &dset), 0);
    BOOST_CHECK( dset != NULL );
}

BOOST_AUTO_TEST_CASE( retrieve_dset_tree )
{
    HdfGroup root("root");
    BOOST_CHECK_EQUAL(root.num_datasets(), 0);
    BOOST_CHECK_EQUAL( root.num_groups(), 0);

    HdfGroup *grp1 = NULL;
    BOOST_CHECK_NO_THROW( grp1 = root.new_group("group1") );
    BOOST_CHECK( grp1 != NULL );
    BOOST_CHECK_NO_THROW( grp1->new_dset("blah1")); // insert some dummy datasets
    BOOST_CHECK_NO_THROW( grp1->new_dset("nnnn1"));

    HdfGroup *grp2 = NULL;
    BOOST_CHECK_NO_THROW( grp2 = grp1->new_group("group2") );
    BOOST_CHECK( grp2 != NULL );
    BOOST_CHECK_NO_THROW( grp2->new_dset("blah2"));
    BOOST_CHECK_NO_THROW( grp2->new_dset("bbbb2"));

    // Create a dset instance to be filled in by reference
    HdfDataset *dset = NULL;
    std::string dset_str("mydset");
    BOOST_CHECK_NO_THROW( dset=grp2->new_dset(dset_str) );
    BOOST_CHECK( dset != NULL );

    //  Create an attribute and attach to the dataset
    std::string attr_name("myattr");
    HdfAttribute attr(attr_name);
    BOOST_CHECK_EQUAL( dset->add_attribute(attr), 0);

    // Finally search the root for the dataset with the named attribute
    HdfDataset *result = NULL;
    BOOST_CHECK_EQUAL( root.find_dset_ndattr(attr_name, &result), 0);
    // and check that the dataset is the very same instance
    BOOST_CHECK_EQUAL( (int)result, (int)dset);
    BOOST_CHECK_EQUAL( result->get_full_name(), dset->get_full_name());
    BOOST_TEST_MESSAGE( "root: " << root._str_() );
    BOOST_TEST_MESSAGE( "dset: " << dset->_str_() );
}

BOOST_AUTO_TEST_CASE( hdfdataset_not_present )
{
    HdfGroup root("root");

    // Search for a non-existing dataset
    std::string dset_str("neverland");
    HdfDataset *dset_ptr = NULL;
    BOOST_CHECK_NE( root.find_dset(dset_str, &dset_ptr), 0);
    BOOST_CHECK( dset_ptr == NULL );

    dset_ptr = NULL;
    BOOST_CHECK_NE( root.find_dset_ndattr(dset_str, &dset_ptr), 0);
    BOOST_CHECK( dset_ptr == NULL );
}

BOOST_AUTO_TEST_SUITE_END()
