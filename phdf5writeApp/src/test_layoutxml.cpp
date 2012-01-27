/*
 * test_layoutxml.cpp
 *
 *  Created on: 26 Jan 2012
 *      Author: up45
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LayoutXmlModule
#include <boost/test/unit_test.hpp>
//#include <boost/test/included/unit_test_framework.hpp> // for a static build use this and comment out BOOST_TEST_DYN_LINK
#include "layout.h"
#include "layoutxml.h"

BOOST_AUTO_TEST_SUITE( simple )

/*
BOOST_AUTO_TEST_CASE(main_xml_layout_xml   )
{
    BOOST_CHECK_EQUAL( main_xml("../../layout.xml"), 0);
}
*/

BOOST_AUTO_TEST_CASE(main_xml_layout_xml   )
{
    LayoutXML l;
    BOOST_CHECK_EQUAL( l.load_xml("layout.xml"), 0);

}

BOOST_AUTO_TEST_SUITE_END()

