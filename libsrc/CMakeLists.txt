

##### Find parallel HDF5 ###################
message("\nSearching for parallel HDF5")

IF (HDF5_ROOT)
  message("HDF5_ROOT set: " ${HDF5_ROOT})
  set( ENV{HDF5_ROOT} ${HDF5_ROOT} )
  find_path( HDF5_INCLUDE_DIRS
             NAMES hdf5.h hdf5_hl.h
             PATHS ${HDF5_ROOT}/include
             NO_DEFAULT_PATH )
  find_library( HDF5_LIBRARIES
                NAMES hdf5 hdf5_hl
                PATHS ${HDF5_ROOT}/lib
                NO_DEFAULT_PATH )
  find_library( HDF5HL_LIBRARIES
                NAMES hdf5_hl
                PATHS ${HDF5_ROOT}/lib
                NO_DEFAULT_PATH )
ELSE(HDF5_ROOT)
    set(HDF5HL_LIBRARIES "")
    find_package(HDF5
                 REQUIRED)

    IF (NOT HDF5_FOUND)
      message(FATAL_ERROR "Unable to find HDF5 installation")
    ELSE (NOT HDF5_FOUND)
      message("HDF5 lib dir:        " ${HDF5_LIBRARY_DIRS})
    ENDIF (NOT HDF5_FOUND)
ENDIF(HDF5_ROOT)

message("HDF5 include files:  " ${HDF5_INCLUDE_DIRS})
message("HDF5 libs:           " ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES})

##### End of HDF5 search ##########


##### Find other dependencies ############
find_package(Log4CXX 0.10.0 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(LibXml2 REQUIRED)

# librt is really only required if glibc =< 2.16
FIND_LIBRARY(REALTIME_LIBRARY
             NAMES rt)
##### End of dependency search ###########


# Include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${HDF5_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${LIBXML2_INCLUDE_DIR})
add_definitions(${HDF5_DEFINITIONS})
add_definitions(${LIBXML2_DEFINITIONS})

# create a list of .cpp files from this directory
file(GLOB phdf5write_SOURCES *.cpp)
set(phdf5write_SOURCES  
                       layoutxml.cpp 
                       profiling.cpp 
                       layout.cpp
                       dimension.cpp
                       NDAttribute.cpp
                       NDArray.cpp
                       writeconfig.cpp)
#                       ndarray_hdf5.cpp
#                       server.cpp )


# Create a library file called helloworld from sources.
# By default it will be static. User can change this by setting
# the cmake variable BUILD_SHARED_LIBS=ON
add_library(phdf5write ${phdf5write_SOURCES})

target_link_libraries(phdf5write ${LOG4CXX_LIBRARIES} ${LIBXML2_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} ${REALTIME_LIBRARY} )

INSTALL(TARGETS phdf5write
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
