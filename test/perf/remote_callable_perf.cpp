#include <arpc/arpc.hpp>


#include <boost/lexical_cast.hpp>

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>



inline std::size_t  dummy_add(const std::size_t v1, const std::size_t v2){
    return v1 + v2;
}


int main(int argc, char** argv)
{
    using namespace arpc::internal;

    std::size_t n = 50000;
    if(argc == 2){
        n = boost::lexical_cast<std::size_t>(std::string(argv[1]));
    }

    std::size_t total = 0;

    std::size_t orig1=0, orig2=1;

    remote_callable<std::size_t, std::size_t, std::size_t> callable(dummy_add);

    auto start = std::chrono::system_clock::now();

    for(decltype(n) i = 0; i < n; i++){
        std::vector<char> buffer = callable.serialize(orig1, orig2);
        std::vector<char> res_buffer = callable.deserialize_and_call(buffer);
        std::size_t res = callable.deserialize_result(res_buffer);


        std::size_t manual_res = dummy_add(orig1, orig2);
        if(manual_res != res){
            std::cerr << "Fatal : incorrect return value" << std::endl;
            exit(1);
        }
        total += res;
        orig1 += 10;
        orig2 += 20;
    }

    auto stop = std::chrono::system_clock::now();

    std::cout << "iterations: " << n << std::endl;
    std::cout << "duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms"<< std::endl;

    std::cout << "dummy res: " << total << std::endl;

}


