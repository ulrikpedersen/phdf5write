/** \cond
 * test_profiling.cpp
 *
 *  Created on: 14 Feb 2012
 *      Author: up45
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ProfilingModule
#include <boost/test/unit_test.hpp>
//#include <boost/test/included/unit_test_framework.hpp> // for a static build use this and comment out BOOST_TEST_DYN_LINK

#include <time.h>
#include "profiling.h"

BOOST_AUTO_TEST_SUITE( ProfilingClass )

BOOST_AUTO_TEST_CASE(constructors)
{
    Profiling pf;
    BOOST_CHECK_EQUAL(pf.count(), 0);

}

BOOST_AUTO_TEST_CASE(tsdiff)
{
    Profiling pf;
    timespec start, end;
    start.tv_sec = 1;
    start.tv_nsec = 0;
    end.tv_sec = 4;
    end.tv_nsec = 3000000;
    BOOST_CHECK_EQUAL( pf.tsdiff(start, end), 3.003);

    end.tv_nsec = 300000000;
    BOOST_CHECK_EQUAL( pf.tsdiff(start, end), 3.3);
    BOOST_CHECK_EQUAL( pf.tsdiff(end, start), -3.3);

    end.tv_nsec = 999000000;
    BOOST_CHECK_EQUAL( pf.tsdiff(start, end), 3.999);
    BOOST_CHECK_EQUAL( pf.tsdiff(end, start), -3.999);

    start.tv_sec = 1;
    start.tv_nsec = 900000000;
    end.tv_sec = 4;
    end.tv_nsec = 300000000;
    BOOST_CHECK_EQUAL( pf.tsdiff(start, end), 2.4);
    BOOST_CHECK_EQUAL( pf.tsdiff(end, start), -2.4);

}

BOOST_AUTO_TEST_CASE(stamps)
{
    Profiling pf;
    int nstamps = 10;
    double stamp = 0.0;
    for (int i = 0; i<nstamps; i++)
    {
        BOOST_CHECK_NO_THROW(stamp = pf.stamp_now());
        BOOST_TEST_MESSAGE("stamp[" << i << "]: "<< stamp);
    }

    BOOST_CHECK_EQUAL( pf.count(), nstamps );
    std::vector<double> delta;
    BOOST_CHECK_NO_THROW( delta = pf.vec_deltatime() )
    BOOST_REQUIRE_EQUAL( delta.size(), pf.vec_timestamps().size() );
    BOOST_TEST_MESSAGE( pf._str() );
}

BOOST_AUTO_TEST_SUITE_END()
/** \endcond */
