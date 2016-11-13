#include <mpi-cpp/mpi.hpp>
#include <arpc/arpc.hpp>


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
    using namespace arpc;

    std::size_t n = 10000;
    if(argc >= 2){
        n = boost::lexical_cast<std::size_t>(std::string(argv[1]));
    }

    std::size_t total = 0;

    std::size_t orig1=0, orig2=1;

    exec_service_mpi service(&argc, &argv);

    remote_function<int, int, int> addition(dummy_add);
    service.register_function(addition);

    mpi::mpi_comm comm;

    auto start = std::chrono::system_clock::now();

    if(comm.rank() == 0) {
        std::vector<int> node_list;
        for(int i = 0; i < comm.size(); i++){
            node_list.push_back(i);
        }

        for(decltype(n) i = 0; i < n; i++){

            std::future<std::vector<int> > future;

            future = addition(node_list, orig1, orig2);

            auto res = future.get();

            for(auto  v : res){
                total += std::size_t(v);
            }
            orig1 += 10;
            orig2 += 20;
        }

        auto stop = std::chrono::system_clock::now();

        const std::size_t nops = n;
        size_t time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() ;
        const double ops_per_sec = double(nops)/(double(time_ms)/1000.0);
        const double avg_latency = (1.0 / ops_per_sec) * 1000.0 * 1000.0 ;


        std::cout << "iterations: " << n << std::endl;
        std::cout << "total_ops: " << nops << std::endl;
        std::cout << "duration: " << time_ms<< "ms"<< std::endl;
        std::cout << "ops_per_seconds: " << ops_per_sec << std::endl;
        std::cout << "avg_global_latency: " << avg_latency << "us" << std::endl;

        std::cout << "dummy res: " << total << std::endl;
    }

    comm.barrier();

}


