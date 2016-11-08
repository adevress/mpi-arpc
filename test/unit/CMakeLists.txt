
if(UNIT_TESTS)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

find_package(Boost 1.41.0 REQUIRED system unit_test_framework)


if(NOT Boost_USE_STATIC_LIBS)
        add_definitions(-DBOOST_TEST_DYN_LINK=TRUE)
endif()




## serialization_tests Test
LIST(APPEND serialization_tests_src "serialization_tests.cpp")

add_executable(serialization_tests_bin ${serialization_tests_src} ${MPI_CPP_HEADERS} ${MPI_CPP_HEADERS_1})
target_link_libraries(serialization_tests_bin  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} ${MPI_LIBRARIES})

add_test(NAME serialization_tests COMMAND ${TESTS_PREFIX} ${TESTS_PREFIX_ARGS} ${CMAKE_CURRENT_BINARY_DIR}/serialization_tests_bin)



## remote_callable_tests Test
LIST(APPEND remote_callable_src "remote_callable_tests.cpp")

add_executable(remote_callable_bin ${remote_callable_src})
target_link_libraries(remote_callable_bin arpc_mpi ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} ${MPI_LIBRARIES})

add_test(NAME remote_callable COMMAND ${TESTS_PREFIX} ${TESTS_PREFIX_ARGS} ${CMAKE_CURRENT_BINARY_DIR}/remote_callable_bin)


## remote_function_tests Test
LIST(APPEND remote_function_src "remote_function_tests.cpp")

add_executable(remote_function_bin ${remote_function_src})
target_link_libraries(remote_function_bin arpc_mpi ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} ${MPI_LIBRARIES})

add_test(NAME remote_function COMMAND ${TESTS_PREFIX} ${TESTS_PREFIX_ARGS} ${CMAKE_CURRENT_BINARY_DIR}/remote_function_bin)


endif()
