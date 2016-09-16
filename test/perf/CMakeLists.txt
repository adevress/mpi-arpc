
if(PERF_TESTS)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

find_package(Boost 1.41.0 REQUIRED system unit_test_framework)


if(NOT Boost_USE_STATIC_LIBS)
        add_definitions(-DBOOST_TEST_DYN_LINK=TRUE)
endif()



## remote_callable_perf_tests Test
LIST(APPEND remote_callable_perf_src "remote_callable_perf.cpp")

add_executable(remote_callable_perf_bin ${remote_callable_perf_src})
target_link_libraries(remote_callable_perf_bin ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} ${MPI_LIBRARIES})



## remote_function_example_perf_tests Test
LIST(APPEND remote_function_example_perf_src "remote_function_example_perf.cpp")

add_executable(remote_function_example_perf_bin ${remote_function_example_perf_src})
target_link_libraries(remote_function_example_perf_bin mpi_arpc ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} ${MPI_LIBRARIES})

## remote_function_example_map_tests Test
LIST(APPEND remote_function_example_map_src "remote_function_example_map.cpp")

add_executable(remote_function_example_map_bin ${remote_function_example_map_src})
target_link_libraries(remote_function_example_map_bin mpi_arpc ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} ${MPI_LIBRARIES})

endif()
