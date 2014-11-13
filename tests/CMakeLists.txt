
### Search for and configure the boost libraries ###
find_package( Boost 1.41.0
              REQUIRED
              COMPONENTS unit_test_framework)



# Include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${HDF5_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${LIBXML2_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS} ../libsrc )
include_directories(${MPI_CXX_INCLUDE_PATH})
add_definitions(${HDF5_DEFINITIONS})
add_definitions(${LIBXML2_DEFINITIONS})

set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_CXX_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_CXX_LINK_FLAGS})


add_executable(test_layout test_layout.cpp)
target_link_libraries(test_layout phdf5write ${Boost_LIBRARIES})

add_executable(test_layoutxml test_layoutxml.cpp)
target_link_libraries(test_layoutxml phdf5write ${Boost_LIBRARIES})

add_executable(test_profiling test_profiling.cpp)
target_link_libraries(test_profiling phdf5write ${Boost_LIBRARIES})

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark phdf5write)

INSTALL(TARGETS test_layout test_layoutxml test_profiling benchmark
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
