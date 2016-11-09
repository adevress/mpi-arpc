#define BOOST_TEST_MODULE remote_callable
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>


#include <mpi-cpp/mpi.hpp>
#include <arpc/arpc.hpp>


#include <iostream>
#include <vector>
#include <fstream>


int argc = boost::unit_test::framework::master_test_suite().argc;
char ** argv = boost::unit_test::framework::master_test_suite().argv;


struct MpiFixture{
    inline MpiFixture():  _env(&argc, &argv){
        _env.enable_exception_report();
    }

    inline ~MpiFixture(){

    }

    mpi::mpi_scope_env _env;
};



BOOST_GLOBAL_FIXTURE( MpiFixture);



int hello_function(const std::string & str){
    mpi::mpi_comm comm;
    std::cout << " " << str << " " << comm.rank() << std::endl;
    return 42;
}



BOOST_AUTO_TEST_CASE( remote_function_local )
{
    std::cout << "start local function test" << std::endl;
    using namespace arpc;

    ::mpi::mpi_comm comm;
    exec_service_mpi pool(&argc, &argv);
    comm.barrier();

    // register the function
    remote_function<int, std::string> hello(hello_function);
    pool.register_function(hello);

    // use as much as you want
    int res = hello(comm.rank(), "hello world ").get();

    std::cout << "answer: " << res << std::endl;


}



BOOST_AUTO_TEST_CASE( remote_function_remote )
{
    std::cout << "start remote function test" << std::endl;
    using namespace arpc;

    mpi::mpi_comm comm;
    exec_service_mpi pool(&argc, &argv);

    if(comm.size() < 2){
        std::cout << "skip this test, requires at least two nodes" << std::endl;
        return;
    }

    // register the function
    remote_function<int, std::string> hello(hello_function);
    pool.register_function(hello);

    // use as much as you want

    std::future<int> future = hello(1, "hello world ");

    std::cout << "answer: " << future.get() << std::endl;


}



