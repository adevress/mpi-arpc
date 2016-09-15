/**
    Copyright (C) 2016, Adrien Devress <adev@adev.name>

        This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as published by
    the Free Software Foundation, either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <unordered_map>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>

#include <mpi-cpp/mpi.hpp>

#include <mpi-arpc/execution_pool.hpp>

namespace mpi {

namespace arpc{


class service_io{
public:
    service_io(MPI_Comm my_comm, const std::function<void (int, int, const std::vector<char> & )> & my_recv_task) :
        poll_thread(),
        executers(),
        recv_task(my_recv_task),
        finished(false),
        comm()
    {
        (void) my_comm;
        const std::size_t n_thread = std::thread::hardware_concurrency();

        poll_thread = std::thread([&]{
            this->poll();
        });

        for(std::size_t i =0; i < n_thread; ++i){
            executers.emplace_back(std::thread([&](){
                this->run();
            }));
        }
    }

    ~service_io(){
        finished = true;

        poll_thread.join();

        for(auto & t : executers){
            t.join();
        }

    }

    void send(int rank, int tag, const std::vector<char> & data){
        comm.send(data, rank, tag);
    }

private:
    service_io(const service_io & ) = delete;

    void run(){
        while(!finished){
            mpi::mpi_comm::message_handle handle;

            {
                std::lock_guard<std::mutex> lock(task_mutex);
                if( handles.size() > 0){
                    handle = std::move(handles.back());
                    handles.pop_back();
                }
            }

            if(handle.is_valid()){
                mpi::mpi_future<std::vector<char> > data = comm.recv_async< std::vector<char> >(handle);
                recv_task(handle.rank(), handle.tag(), data.get());
            }

            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }


    void poll(){
        while(!finished){
            mpi::mpi_comm::message_handle handle = comm.probe(mpi::any_source, mpi::any_tag, 0);
            if(handle.is_valid()){
                std::lock_guard<std::mutex> lock(task_mutex);
                handles.emplace_back(handle);
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }


    std::mutex task_mutex;
    std::vector<mpi::mpi_comm::message_handle> handles;


    std::thread poll_thread;
    std::vector<std::thread> executers;

    std::function<void (int, int, const std::vector<char> &)> recv_task;

    bool finished;

    mpi::mpi_comm comm;
};


class execution_pool_pthread::pimpl {
public:
    pimpl(int* argc, char*** argv) :
        env(argc, argv),
        io(MPI_COMM_WORLD, [&] (int rank, int id, const std::vector<char> & data) {
            this->recv_handler(rank, id, data);
    }) {}


    void recv_handler(int rank, int id, const std::vector<char> & data){
        std::cout << "recv msg from friend " << rank << " id " << id << " data " << std::string(data.data(), data.size()) <<std::endl; ;
    }


    std::unordered_map<std::string, std::shared_ptr<internal::callable_object> > function_map;

    mpi::mpi_scope_env env;
    service_io io;
};


execution_pool_pthread::execution_pool_pthread(int* argc, char*** argv): d_ptr(new pimpl(argc, argv)) {}

execution_pool_pthread::~execution_pool_pthread() {}


void execution_pool_pthread::register_function_internal(const std::string & function_name, std::shared_ptr<internal::callable_object> && callable){
    if(d_ptr->function_map.insert(std::make_pair(function_name, callable)).second != true){
        throw std::runtime_error(std::string("registered function named '") + function_name + "'' already exist" );
    }
}


std::shared_ptr<internal::callable_object> execution_pool_pthread::resolve_function_internal(const std::string & function_name){
    auto it = d_ptr->function_map.find(function_name);
    if(it == d_ptr->function_map.end()){
        throw std::runtime_error(std::string("no registered function named '") + function_name + "''" );
    }

    return it->second;
}




}; // arpc


}; // mpi



