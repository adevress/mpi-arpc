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
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <cstddef>

#include <mpi-cpp/mpi.hpp>

#include <mpi-arpc/execution_pool.hpp>

namespace mpi {

namespace arpc{

namespace {

constexpr int tag_range1_begin = 2;
constexpr int tag_range1_end = tag_range1_begin+ std::numeric_limits<int>::max()/4;
constexpr int tag_range2_begin = tag_range1_end;
constexpr int tag_range2_end = tag_range1_end + std::numeric_limits<int>::max()/4;


template<typename Fun>
class request_stack{
public:

    int register_req(const Fun & req){
        std::lock_guard<std::mutex> lock(mutex_request);

        requests.push_back(req);
        return tag_range2_begin + requests.size() -1;
    }

    Fun & get_request_from_id(int id){
        assert(id >= tag_range2_begin);

        std::lock_guard<std::mutex> lock(mutex_request);
        const size_t offset = id - tag_range2_begin;
        assert((offset)  < requests.size());
        assert(requests[offset] != nullptr);
        return requests[offset];
    }

    void pop_request(int id){
        assert(id >= tag_range2_begin);

        std::lock_guard<std::mutex> lock(mutex_request);

        const size_t offset = id - tag_range2_begin;
        assert((offset)  < requests.size());

        // invalidate the request
        requests[offset] = nullptr;

        // pop every last invalid request in stack mode
        while(requests.size() > 0 && requests.back() == nullptr){
            requests.pop_back();
        }

    }

private:
    std::vector<Fun> requests;
    std::mutex mutex_request;
};


}


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
        comm.barrier();

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
        comm.barrier();
        finished = true;

        poll_thread.join();

        for(auto & t : executers){
            t.join();
        }

    }

    inline void send(int rank, int tag, const std::vector<char> & data){
        comm.send(data, rank, tag);
    }

    inline void barrier(){
        comm.barrier();
    }

    inline mpi_comm & get_comm(){
        return comm;
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
                continue;
            }

            {
                std::unique_lock<std::mutex> lock(task_mutex);
                task_cond.wait_for(lock, std::chrono::microseconds(50));
            }
        }
    }


    void poll(){
        while(!finished){
            mpi::mpi_comm::message_handle handle = comm.probe(mpi::any_source, mpi::any_tag, 1);
            if(handle.is_valid()){
                std::lock_guard<std::mutex> lock(task_mutex);
                handles.emplace_back(std::move(handle));
                task_cond.notify_one();
            }
        }
    }


    std::mutex task_mutex;
    std::condition_variable task_cond;
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
        }),
        n(tag_range1_begin) {}


    void recv_handler(int rank, int id, const std::vector<char> & data){

        int callable_id = id & 0xffff;
        int request_id = ((id >> 16) & 0xffff) + tag_range2_begin;

        //std::cout << "dest from " << rank << " callable_id " << callable_id << " request_id " << request_id <<std::endl;

        if(callable_id == 0){ // response
            req_stack.get_request_from_id(request_id)(data);
            req_stack.pop_request(request_id);
        }else{
            try{
               int new_tag = id & (~0xffff);
               std::vector<char> serialized_result = int_to_function_map[callable_id]->deserialize_and_call(data);
               io.send(rank, new_tag, serialized_result);
            }catch(std::exception & e){
                std::cerr << "<exception> on rank " << io.get_comm().rank()
                          << " with request from rank " << rank << " " << e.what() << std::endl;
            }
        }

    }


    std::mutex map_locker;

    std::unordered_map<int, std::shared_ptr<internal::callable_object> > int_to_function_map;

    std::unordered_map<std::shared_ptr<internal::callable_object>, int > function_to_in_map;


    request_stack<std::function< void(const std::vector<char> &)> > req_stack;


    mpi::mpi_scope_env env;
    service_io io;
    std::size_t n;
};


execution_pool_pthread::execution_pool_pthread(int* argc, char*** argv): d_ptr(new pimpl(argc, argv)) {}

execution_pool_pthread::~execution_pool_pthread() {}


int execution_pool_pthread::register_function_internal(std::shared_ptr<internal::callable_object> callable){
    std::size_t id = d_ptr->n +1;

    {
        std::lock_guard<std::mutex> lock(d_ptr->map_locker);

        if(d_ptr->int_to_function_map.insert(std::make_pair(id, callable)).second != true){
            throw std::runtime_error(std::string("registered function with id '") + std::to_string(id) + "'' already exist" );
        }

        if(d_ptr->function_to_in_map.insert(std::make_pair(callable, id)).second != true){
            throw std::runtime_error(std::string("registered function. ptr already registered ") + std::to_string(ptrdiff_t(callable.get())) );
        }

        d_ptr->n = id;
    }

    d_ptr->io.barrier();
    return id;
}


bool execution_pool_pthread::is_local(int rank){
    return d_ptr->io.get_comm().rank() == rank;
}

void execution_pool_pthread::send_request(int rank, int callable_id, const std::vector<char> & args_serialized,
                  const std::function<void (const std::vector<char> &)> & callback){

    int req_id = d_ptr->req_stack.register_req(callback);

    //std::cout << "source from " << rank << " callable_id " << callable_id << " request_id " << req_id <<std::endl;

    assert(callable_id < 0xffff);
    int tag = (callable_id & 0xffff);
    tag |= ((req_id - tag_range2_begin) << 16);

    d_ptr->io.send(rank, tag, args_serialized);
}



}; // arpc


}; // mpi



