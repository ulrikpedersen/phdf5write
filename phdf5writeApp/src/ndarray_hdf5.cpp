/*
 * ndarray_hdf5.cpp
 *
 *  Created on: 6 Jan 2012
 *      Author: up45
 */

#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

#include <NDArray.h>
#include "dimension.h"
#include "ndarray_hdf5.h"


int NDArrayToHDF5::h5_open(const char *filename)
{
    int retcode = 0;
    string fname(filename);
    return retcode;
}

int NDArrayToHDF5::h5_write(NDArray& ndarray)
{
    int retcode = 0;

    this->conf.next_frame(ndarray);
    msg(this->conf._str_());

    return retcode;
}

int NDArrayToHDF5::h5_close()
{
    int retcode = 0;

    return retcode;
}
