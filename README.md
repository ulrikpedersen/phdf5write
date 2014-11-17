phdf5write
==========
[![Build Status](https://travis-ci.org/ulrikpedersen/phdf5write.svg?branch=cmake)](https://travis-ci.org/ulrikpedersen/phdf5write)

A parallel HDF5 file writer service application with benchmarking.

Building
--------

Before building, please ensure that you have the dependencies installed. You will
need the following packages including their header files (-dev or -devel packages):

* cmake (>=2.8)
* openmpi (or alternative MPI package)
* hdf5 - the parallel build
* log4cxx
* boost (unittest framework)

Clone/checkout the phdf5writer from github.

    cd phdf5write
    mkdir build
    cd build
    cmake ..
    make

Building on the DLS RHEL6 environment caused some trouble. Currently we can build it with the following tweaks:

    cmake -DHDF5_ROOT=/dls_sw/work/tools/RHEL6-x86_64/phdf5/prefix \
          -DLOG4CXX_ROOT_DIR=/dls_sw/prod/tools/RHEL6-x86_64/log4cxx/0-10-0/prefix \
          -DLOG4CXX_INCLUDE_DIRS:PATH=/dls_sw/prod/tools/RHEL6-x86_64/log4cxx/0-10-0/prefix/include \
          -DHDF5_USE_STATIC_LIBRARIES=ON \
          -DMPI_CC_COMPILER=/dls_sw/work/tools/RHEL6-x86_64/openmpi/prefix/bin/mpicc \
          -DMPI_CXX_COMPILER=/dls_sw/work/tools/RHEL6-x86_64/openmpi/prefix/bin/mpic++ \
          ..

Running the benchmark
---------------------

The benchmark application takes three commandline arguments all of which are paths to XML configuration files:
* A test configuration file
* HDF5 layout definition file
* A log4cxx configuration file

Example configuration files are provide in the conf/ folder. Running the basic sanity check is simple like this:

    build/bin/benchmark conf/testconf.xml conf/layout.xml conf/Log4cxxConfig.xml

