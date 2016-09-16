
#include <mpi-cpp/mpi.hpp>
#include <mpi-arpc/mpi-arpc.hpp>


#include <boost/lexical_cast.hpp>

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>



inline int  dummy_add(const int v1, const int v2){
    mpi::mpi_comm comm;
    std::cout << "** doing addition on node " << comm.rank() << std::endl;
    int res =  v1 + v2;
    std::cout << "** sending the result to the requester "  << std::endl;
    return res;
}


int main(int argc, char** argv)
{
   using namespace mpi::arpc;

   execution_pool_pthread service(&argc, &argv);

   remote_function<int, int, int> addition(dummy_add);
   service.register_function(addition);

   mpi::mpi_comm comm;

   if(comm.rank() ==0){
        std::cout << "** calling an addition on node 1 from node 0" << std::endl;
        std::future<int> future_result = addition(1, 40, 2);

        std::cout << "** non-blocking call ! " << std::endl;
        std::cout << "addition result: "  << future_result.get() << std::endl;
   }

    comm.barrier();
}


