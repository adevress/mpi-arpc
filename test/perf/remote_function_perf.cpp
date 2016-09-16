#include <mpi-cpp/mpi.hpp>
#include <mpi-arpc/mpi-arpc.hpp>


#include <boost/lexical_cast.hpp>

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>



inline int  dummy_add(const int v1, const int v2){
    mpi::mpi_comm comm;
    int res =  v1 + v2;
    return res;
}


int main(int argc, char** argv)
{
    using namespace mpi::arpc;

    std::size_t n = 100;
    if(argc >= 2){
        n = boost::lexical_cast<std::size_t>(std::string(argv[1]));
    }

    std::size_t n_parallel = 1;
    if(argc >= 3){
        n_parallel = boost::lexical_cast<std::size_t>(std::string(argv[2]));
    }

    std::size_t total = 0;

    std::size_t orig1=0, orig2=1;

    execution_pool_pthread service(&argc, &argv);

    remote_function<int, int, int> addition(dummy_add);
    service.register_function(addition);

    mpi::mpi_comm comm;

    auto start = std::chrono::system_clock::now();

    if(comm.rank() == 0) {
        for(decltype(n) i = 0; i < n; i++){

            std::vector< std::future<int> > vec_future;

            for(std::size_t i =0; i < n_parallel; ++i){
                vec_future.emplace_back( addition(1, orig1, orig2) );
            }

            for(auto & fut : vec_future){
                total += fut.get();
            }
            orig1 += 10;
            orig2 += 20;
        }

        auto stop = std::chrono::system_clock::now();

        std::cout << "iterations: " << n << std::endl;
        std::cout << "parallel_ops: " << n_parallel << std::endl;
        const std::size_t nops = n_parallel*n;
        std::cout << "total_ops: " << nops << std::endl;
        size_t time_ms =   std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() ;
        std::cout << "duration: " << time_ms<< "ms"<< std::endl;
        std::cout << "ops_per_seconds: " << double(nops)/(double(time_ms)/1000.0)<< std::endl;


        std::cout << "dummy res: " << total << std::endl;
    }

    comm.barrier();

}


