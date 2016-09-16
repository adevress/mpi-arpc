
#include <mpi-cpp/mpi.hpp>
#include <mpi-arpc/mpi-arpc.hpp>


#include <boost/lexical_cast.hpp>

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <map>


const std::vector<std::string> fruits = { "apple", "pear", "grappe", "lemon", "orange", "pikachu" };

inline const std::map<std::string, std::string>  fill_map(const std::map<std::string, std::string> & in){
    mpi::mpi_comm comm;

    std::map<std::string, std::string> res = in;

    res.insert(std::make_pair<std::string,std::string>(std::to_string(comm.rank()),
                                                        std::string(fruits[comm.rank()%fruits.size()])));

    std::cout << "** sending the result to the requester from node " << comm.rank()  << std::endl;
    return res;
}


int main(int argc, char** argv)
{
   using namespace mpi::arpc;

   execution_pool_pthread service(&argc, &argv);

   remote_function<std::map<std::string, std::string>, std::map<std::string, std::string> > fill_map_function(fill_map);
   service.register_function(fill_map_function);

   mpi::mpi_comm comm;

   if(comm.rank() ==0){
        std::map<std::string, std::string> res;

        for(int i=0; i <comm.size(); ++i){
            res = fill_map_function(i, res).get();
        }

        for(auto & elem : res){
            std::cout << "insertered element: " << elem.first << " " << elem.second <<std::endl;
        }
   }

    comm.barrier();
}


