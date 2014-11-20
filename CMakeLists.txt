# swmr-testapp project cmake build system
# 
# This project builds a single executable binary using a standard cmake build.
#
# Build instructions:
#   cd phdf5write
#   mkdir build
#   cmake ..
#   make
#   sudo make install
#
# The cmake command will search for the dependencies in the usual places. If you
# have custom installations of these libraries and headers, you can tell cmake
# about it with the usual macros (cmake -D<macro>=/path/to/installation ..)
#
# The supported macros for the relevant dependencies are:
#   HDF5_ROOT
#   BOOST_ROOT
#   LOG4CXX_ROOT_DIR


# Project name 
project(phdf5write)

# States that CMake required version must be greater than 2.8
cmake_minimum_required(VERSION 2.8)

option(WITH_MPI "Build project in parallel mode with MPI" ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Appends the cmake/modules path inside the MAKE_MODULE_PATH variable which stores the
# directories of additional CMake modules (ie. MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${phdf5write_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

find_package(Log4CXX 0.10.0 REQUIRED)

add_subdirectory(libsrc)
add_subdirectory(tests)

