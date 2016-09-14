#define BOOST_TEST_MODULE remote_callable
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>


#include <mpi-arpc/mpi-arpc.hpp>


#include <iostream>
#include <vector>
#include <fstream>


int argc = boost::unit_test::framework::master_test_suite().argc;
char ** argv = boost::unit_test::framework::master_test_suite().argv;


int print_args(const std::string & str0, const std::string & str1){
    std::cout << " 0 " << str0 << " 1 " << str1 << std::endl;
    BOOST_CHECK_EQUAL(str0, "hello");
    BOOST_CHECK_EQUAL(str1, "world");
    return 42;
}



BOOST_AUTO_TEST_CASE( remote_callable_basic )
{
    using namespace mpi::arpc::internal;

    remote_callable<int, std::string, std::string> callable(print_args);

    std::vector<char> buffer = callable.serialize("hello", "world");


    std::vector<char> res = callable.deserialize_and_call(buffer);

    int result = callable.deserialize_result(res);

    BOOST_CHECK_EQUAL(result, 42);

}


BOOST_AUTO_TEST_CASE( remote_callable_lambda )
{
    using namespace mpi::arpc::internal;
    const std::string extern_msg = "bob! ";

    remote_callable<int, std::string, std::string> callable([&](const std::string & str1, const std::string & str2) {
        std::cout << "test " << str1 << " " << str2 << " " << extern_msg << std::endl;
        return 42;
    });

    std::vector<char> buffer = callable.serialize("hello", "world");


    std::vector<char> res = callable.deserialize_and_call(buffer);

}


