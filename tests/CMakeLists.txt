
### Search for and configure the boost libraries ###
find_package( Boost 1.41.0
              REQUIRED
              COMPONENTS unit_test_framework)



# Include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${HDF5_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${LIBXML2_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS} ../libsrc )
add_definitions(${HDF5_DEFINITIONS})
add_definitions(${LIBXML2_DEFINITIONS})

add_executable(test_layout test_layout.cpp)
target_link_libraries(test_layout phdf5write ${Boost_LIBRARIES})

add_executable(test_layoutxml test_layoutxml.cpp)
target_link_libraries(test_layoutxml phdf5write ${Boost_LIBRARIES})

add_executable(test_profiling test_profiling.cpp)
target_link_libraries(test_profiling phdf5write ${Boost_LIBRARIES})


INSTALL(TARGETS test_layout test_layoutxml test_profiling
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
