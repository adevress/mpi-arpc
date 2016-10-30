#include <mpi-cpp/mpi.hpp>
#include <mpi-arpc/mpi-arpc.hpp>


#include <boost/lexical_cast.hpp>

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>


typedef std::vector<char> vector_elems;
using namespace mpi::arpc;

struct exec_context{
    exec_context() :
        iterations(1000),
        n_parallel(1),
        elem_size(1),
        min_time(1000000),
        max_time(0),
        av_time(0) {}


    std::size_t iterations, n_parallel;
    std::size_t elem_size;
    double min_time, max_time, av_time;
};


inline vector_elems just_return(const vector_elems & args){
    vector_elems res(args);
    return res;
}


void execute_ping_pong(exec_context & c, remote_function<vector_elems, vector_elems> & executor){
    std::size_t total = 0;
    std::size_t dest_node = 1;

    vector_elems elems;
    mpi::mpi_comm comm;

    for(std::size_t i =0; i < c.elem_size; ++i){
        elems.push_back(char(i));
    }

    if(comm.rank() == 0) {

        for(std::size_t i = 0; i < c.iterations; i++){
            auto start = std::chrono::system_clock::now();

            std::vector< std::future<vector_elems> > vec_future;

            for(std::size_t i =0; i < c.n_parallel; ++i){
                vec_future.emplace_back( executor(dest_node, elems) );
            }

            for(auto & fut : vec_future){
                total += fut.get().size();
            }

           auto stop = std::chrono::system_clock::now();
           double time_us = double(std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count()) / 1000.0 ;
           c.min_time = std::min(c.min_time, time_us);
           c.max_time = std::max(c.max_time, time_us);
           c.av_time += time_us;
        }

    }
    c.av_time /= c.iterations;

}

int main(int argc, char** argv)
{

    std::size_t max_elem_size = 65536;

    execution_pool_pthread service(&argc, &argv);
    mpi::mpi_comm comm;

    remote_function<vector_elems, vector_elems> ping_pong(just_return);
    service.register_function(ping_pong);


    if(comm.rank() == 0) {
        std::cout << "\t\t#bytes\t\t#repetitions\t\tt[usec]\t\tmint[usec]\t\tmaxt[usec]\t\tMbytes/sec\n";
    }



    std::size_t elem_size = 1;

    while(elem_size <= max_elem_size){
        exec_context context;
        context.elem_size = elem_size;
        execute_ping_pong(context, ping_pong);

        if(comm.rank() == 0) {
            std::cout << "\t\t" << context.elem_size
                  << "\t\t" << context.iterations
                  << "\t\t" << context.av_time
                  << "\t\t" << context.min_time
                  << "\t\t" << context.max_time
                  << "\t\t" << (context.elem_size) / context.av_time
                  << "\n";
        }
        elem_size= elem_size << 1;
    }

    std::cout << std::endl;

    comm.barrier();

}


