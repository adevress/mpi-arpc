#include <mpi-cpp/mpi.hpp>
#include <arpc/arpc.hpp>


#include <boost/lexical_cast.hpp>

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <future>
#include <atomic>



inline int  dummy_add(const int v1, const int v2){
    mpi::mpi_comm comm;
    int res =  v1 + v2;
    return res;
}


int main(int argc, char** argv)
{
    using namespace arpc;

    mpi::mpi_scope_env mpi_env(&argc, &argv);

    std::size_t n = 100;
    if(argc >= 2){
        n = boost::lexical_cast<std::size_t>(std::string(argv[1]));
    }

    std::size_t n_parallel = 1;
    if(argc >= 3){
        n_parallel = boost::lexical_cast<std::size_t>(std::string(argv[2]));
    }

    std::atomic<std::size_t> total(0);

    std::size_t orig1=0, orig2=1;

    mpi::mpi_comm comm;

    typedef std::vector<std::future<int> > futures_vector;

    auto start = std::chrono::steady_clock::now();

    if(comm.rank() == 0) {
        std::vector<std::thread> res;
        for(std::size_t t =0; t < n_parallel; ++t){
            res.emplace_back(std::thread( [t, n, orig1, orig2, &total] {
            std::size_t v1 = orig1, v2 = orig2, local_total=0;
            futures_vector all_vec;
            all_vec.reserve(n);
            for(decltype(n) i = 0; i < n; i++){
               all_vec.emplace_back(  std::async(std::launch::async, [v1, v2]{ return dummy_add(v1, v2); } ));
               v1 += 10;
               v2 += 20;
            }

            for(auto & fut : all_vec){
                local_total += static_cast<std::size_t>(fut.get());
            }
            total += local_total;

            }));

        }

        for(auto & i : res){
            i.join();
        }

        auto stop = std::chrono::steady_clock::now();

        std::cout << "iterations: " << n << std::endl;
        std::cout << "parallel_ops: " << n_parallel << std::endl;
        const std::size_t nops = n_parallel*n;
        std::cout << "total_ops: " << nops << std::endl;
        size_t time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() ;
        std::cout << "duration: " << time_ms<< "ms"<< std::endl;
        std::cout << "ops_per_seconds: " << double(nops)/(double(time_ms)/1000.0)<< std::endl;


        std::cout << "dummy res: " << total << std::endl;

    }

    comm.barrier();

}


