/** \cond
 * test_dimension.cpp
 *
 *  Created on: 10 Jan 2012
 *      Author: up45
 *
 *  Using a rather old version (1.33) of boost that is shipped with RHEL5.
 *  This version supposedly has support for things like module name, suites,
 *  and fixtures. However, these features does not seem to work in this early version.
 *  So only simple test cases are used here.
 */

// Boost.Test
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

#include "dimension.h"

using namespace phdf5;

/*BOOST_AUTO_TEST_SUITE( dimension_test_suite )*/

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES( test1, 1 )
BOOST_AUTO_TEST_CASE( test1 )
{
    //BOOST_CHECK( true );
    DimensionDesc dim;
    //BOOST_CHECK(dim.element_size == 1);
    BOOST_CHECK_MESSAGE(dim.element_size == 1, "element size should be 0");
}

BOOST_AUTO_TEST_CASE( test2 )
{
    BOOST_CHECK( true );
}

/*BOOST_AUTO_TEST_SUITE_END()*/
/** \endcond */
