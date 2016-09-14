
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




endif()
