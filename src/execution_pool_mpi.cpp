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
#include <cstdint>
#include <cstring>

#include <mpi-cpp/mpi.hpp>

#include <arpc/execution_pool_mpi.hpp>

namespace arpc {


namespace {

const std::uint8_t message_type_request = 0x01;
const std::uint8_t message_type_answer = 0x02;
const std::uint8_t message_type_exception = 0x03;


struct message_header{
    message_header() :
        request_id(0),
        identifier_token(0),
        message_type(0),
        source(-1){}

    std::uint64_t request_id;
    std::uint32_t identifier_token;
    std::uint8_t message_type;

    // source is not transmitted, but added by the MPI layer
    int source;

    std::vector<char> serialize(){
        std::vector<char> res;
        res.resize(serialized_data_size);
        char * pbuffer = res.data();
        *((decltype(request_id)*) pbuffer) = request_id;
        pbuffer+= sizeof(request_id);
        *((decltype(identifier_token)*) pbuffer) = identifier_token;
        pbuffer += sizeof(identifier_token);
        *((decltype(message_type)*) pbuffer) = message_type;
        return res;
    }

    void deserialize(const std::vector<char> & vec){
        if(vec.size() != serialized_data_size){
            throw std::logic_error("Invalid header message, length inconsistency");
        }

        char * pbuffer = const_cast<char*>(vec.data());
        request_id = *((decltype(request_id)*) pbuffer);
        pbuffer+= sizeof(request_id);
        identifier_token = *((decltype(identifier_token)*) pbuffer);
        pbuffer += sizeof(identifier_token);
        message_type = *((decltype(message_type)*) pbuffer);
    }

    static constexpr std::size_t serialized_data_size =
            sizeof(decltype(request_id)) + sizeof(decltype(identifier_token))
            + sizeof(decltype(message_type));

};

constexpr int tag_range1_begin = 2;
constexpr int tag_range1_end = tag_range1_begin+ std::numeric_limits<int>::max()/4;
constexpr int tag_range2_begin = tag_range1_end;
constexpr int tag_range2_end = tag_range1_end + std::numeric_limits<int>::max()/4;


template<typename ResultHandler>
class request_stack{
public:

    int register_req(std::unique_ptr<ResultHandler> && req){
        std::lock_guard<std::mutex> lock(mutex_request);

        requests.emplace_back(std::move(req));
        return tag_range2_begin + requests.size() -1;
    }

    ResultHandler& get_request_from_id(int id){
        assert(id >= tag_range2_begin);

        std::lock_guard<std::mutex> lock(mutex_request);
        const size_t offset = id - tag_range2_begin;
        assert((offset)  < requests.size());
        assert(requests[offset] != nullptr);
        return *requests[offset];
    }

    void pop_request(int id){
        assert(id >= tag_range2_begin);

        std::lock_guard<std::mutex> lock(mutex_request);

        const size_t offset = id - tag_range2_begin;
        assert((offset)  < requests.size());

        // invalidate the request
        requests[offset].reset();

        // pop every last invalid request in stack mode
        while(requests.size() > 0 && requests.back().get() == nullptr){
            requests.pop_back();
        }

    }

private:
    std::vector<typename std::unique_ptr<ResultHandler>> requests;
    std::mutex mutex_request;
};


}


class service_io{
public:
    using vector_req_status = std::vector<mpi::mpi_future<std::vector<char>> >;
    using req_status = mpi::mpi_future<std::vector<char>>;

    service_io(MPI_Comm my_comm, const std::function<void (int, message_header &, const std::vector<char> & )> & my_recv_task) :
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
        return comm.send(data, rank, tag);
    }

    inline req_status send_async(int rank, int tag, const std::vector<char> & data){
        return comm.send_async(data, rank, tag);
    }


    inline vector_req_status send_bulk(const std::vector<int> & node_list, int tag, const std::vector<char> & data){
        vector_req_status futures;
        futures.reserve(node_list.size());

        for(auto & node : node_list){
            futures.emplace_back(std::move(comm.send_async(data, node, tag)));
        }

        return futures;
    }

    inline void barrier(){
        comm.barrier();
    }

    inline int get_rank() const{
        return comm.rank();
    }

    inline ::mpi::mpi_comm & get_comm(){
        return comm;
    }

private:
    service_io(const service_io & ) = delete;

    void run(){
        while(!finished){
            message_header handle;

            {
                std::lock_guard<std::mutex> lock(task_mutex);
                if( handles.size() > 0){
                    handle = std::move(handles.back());
                    handles.pop_back();
                }
            }

            if(handle.identifier_token != 0){
                ::mpi::mpi_comm::message_handle data_handle = comm.probe(handle.source, 2);
                if(data_handle.is_valid()){
                    ::mpi::mpi_future<std::vector<char> > data = comm.recv_async< std::vector<char> >(data_handle);
                    recv_task(data_handle.rank(), handle, data.get());
                }
                continue;
            }

            {
                std::unique_lock<std::mutex> lock(task_mutex);
                task_cond.wait_for(lock, std::chrono::microseconds(100));
            }
        }
    }


    void poll(){
        while(!finished){
            ::mpi::mpi_comm::message_handle handle = comm.probe(::mpi::any_source, 1, 1);
            if(handle.is_valid()){
                message_header headers;
                std::vector<char> headers_data;
                comm.recv(handle, headers_data);
                headers.deserialize(headers_data);
                headers.source = handle.rank();

                std::lock_guard<std::mutex> lock(task_mutex);
                handles.emplace_back(std::move(headers));
                task_cond.notify_one();
            }
        }
    }


    std::mutex task_mutex;
    std::condition_variable task_cond;
    std::vector<message_header> handles;


    std::thread poll_thread;
    std::vector<std::thread> executers;

    std::function<void (int, message_header &, const std::vector<char> &)> recv_task;

    bool finished;

    ::mpi::mpi_comm comm;
};


class exec_service_mpi::pimpl {
public:
    pimpl(int* argc, char*** argv) :
        env(argc, argv),
        io(MPI_COMM_WORLD, [&] (int rank, message_header& header, const std::vector<char> & data) {
            this->recv_handler(rank, header, data);
        }),
        n(tag_range1_begin) {}


    void recv_handler(int rank, message_header & headers, const std::vector<char> & data){
        int callable_id = headers.request_id;
        int request_id = headers.identifier_token;

        std::ostringstream ss;
        /*ss << "on " <<  io.get_rank() << " " <<
                   "dest from " << rank << " callable_id " << callable_id << " request_id " << request_id
                  << " msg_type " << int(headers.message_type) << " size " << data.size();
        std::cout << ss.str() << std::endl;*/

        if(headers.message_type == message_type_answer){ // response
            //std::cout << "execute answer " << std::endl;

            internal::result_object& req = req_stack.get_request_from_id(request_id);
            const bool completed = req.add_result(data);
             if(completed){
                req_stack.pop_request(request_id);
            }
        }else if(headers.message_type == message_type_request){
            try{
               //std::cout << "execute request " <<  data.size() << " " << data.data() << std::endl;
               std::vector<char> serialized_result = int_to_function_map[callable_id]->deserialize_and_call(data);

               message_header response_headers;
               response_headers.identifier_token = request_id;
               response_headers.request_id = callable_id;
               response_headers.message_type = message_type_answer;

               std::vector<char> headers_data = response_headers.serialize();
               auto f_header = io.send_async(rank, 1, headers_data);
               auto f_data = io.send_async(rank, 2, serialized_result);
               f_header.wait();
               f_data.wait();
            }catch(std::exception & e){
                std::cerr << "<exception> on rank " << io.get_comm().rank()
                          << " with request from rank " << rank << " " << e.what() << std::endl;
            }
        }else{
            std::cerr << "Error: recv message with unknown message type" << headers.message_type << "\n";
        }

    }


    std::mutex map_locker;

    std::unordered_map<int, std::shared_ptr<internal::callable_object> > int_to_function_map;

    std::unordered_map<std::shared_ptr<internal::callable_object>, int > function_to_in_map;


    request_stack<internal::result_object> req_stack;


    ::mpi::mpi_scope_env env;
    service_io io;
    std::size_t n;
};


exec_service_mpi::exec_service_mpi(int* argc, char*** argv): d_ptr(new pimpl(argc, argv)) {}

exec_service_mpi::~exec_service_mpi() {}


int exec_service_mpi::register_function_internal(std::shared_ptr<internal::callable_object> callable){
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


bool exec_service_mpi::is_local(int rank){
    return d_ptr->io.get_comm().rank() == rank;
}

void exec_service_mpi::send_request(int rank, int callable_id, const std::vector<char> & args_serialized,
                  std::unique_ptr<internal::result_object> && result_handler){

    message_header headers;
    headers.identifier_token = d_ptr->req_stack.register_req(std::move(result_handler));
    headers.request_id = callable_id;
    headers.message_type = message_type_request;

    auto header_data = headers.serialize();


    auto future_header = d_ptr->io.send_async(rank, 1, header_data);
    auto future_data = d_ptr->io.send_async(rank, 2, args_serialized);

    future_header.wait();
    future_data.wait();
}

void exec_service_mpi::send_request(std::vector<int> node_list, int callable_id, const std::vector<char> &args_serialized, std::unique_ptr<internal::result_object> &&result_handler){
    message_header headers;
    headers.identifier_token = d_ptr->req_stack.register_req(std::move(result_handler));
    headers.request_id = callable_id;
    headers.message_type = message_type_request;

    auto header_data = headers.serialize();

    auto all_future_headers = d_ptr->io.send_bulk(node_list, 1, header_data);
    auto all_futures = d_ptr->io.send_bulk(node_list, 2, args_serialized);

    for(auto & f : all_future_headers){
        f.wait();
    }
    for(auto & f : all_futures){
        f.wait();
    }

}





}; // arpc



