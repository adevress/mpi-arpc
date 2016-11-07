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
#ifndef REMOTE_FUNCTION_HPP
#define REMOTE_FUNCTION_HPP

#include <functional>
#include <memory>
#include <future>

#include "bits/remote_callable.hpp"


struct arpc_unit_tests;

namespace mpi {

namespace arpc{

class execution_pool_pthread;

///
/// \brief remote_function
///
///  contain a function that can be called on a remote node
///
///
template<typename Ret, typename... Args>
class remote_function {
public:

    typedef Ret result_type;

    remote_function(const std::function<Ret(Args...)> & function_object) :
        _callable(std::make_shared<internal::remote_callable<Ret, Args... > >(function_object)),
        _callable_id(0),
        _pool(nullptr){

    }

    virtual ~remote_function(){};


    std::future<result_type> operator()(int rank, Args... args){
        check_service_association();
        return _execute_async(rank, std::forward<Args>(args)...);
    }


private:
    remote_function(const remote_function &) = delete;

    inline void check_service_association(){
        if(_pool == nullptr){
            throw std::runtime_error("no service associated to this remote_function");
        }
    }

    std::future<result_type> _execute_async(int rank, Args... args){



        // if request is local to node, execute directly
        if(_pool->is_local(rank)){
            return _execute_async_local_serialize(std::forward<Args>(args)...);
        }

        const std::shared_ptr<internal::remote_callable<Ret, Args...> > & callback_callable = _callable;
        auto prom = std::make_shared<std::promise<result_type> >();
        auto future_result = prom->get_future();

        std::vector<char> args_serialized = _callable->serialize(args...);
        _pool->send_request(rank, _callable_id, args_serialized, std::function<void (const std::vector<char> &)>(
                                [prom, callback_callable] (const std::vector<char> & result ) {
                                    //std::cout << "back on earth " << std::string(result.data(), result.size()) << " " << result.size() << std::endl;
                                    result_type res = callback_callable->deserialize_result(result);
                                    prom->set_value(res);
                                })
                            );
        return future_result;
    }


    std::future<result_type> _execute_async_local(Args... args){
        const std::shared_ptr<internal::remote_callable<Ret, Args...> > & callback_callable = _callable;

        using type_tuple =  typename internal::remote_callable<Ret, Args...>::type_tuple;

        std::shared_ptr<type_tuple> fwd_args_tuple(new type_tuple(std::forward<Args>(args)...));

        return std::async(std::launch::deferred, [fwd_args_tuple, callback_callable](){
            return callback_callable->call_from_tuple(*fwd_args_tuple);
        });
    }

    inline std::future<result_type> _execute_async_local_serialize(Args... args){

        std::vector<char> args_serialized = _callable->serialize(args... );
        std::vector<char> res_serialized = _callable->deserialize_and_call(args_serialized);

        std::promise<result_type> prom;
        std::future<result_type> fut = prom.get_future();
        prom.set_value(_callable->deserialize_result(res_serialized));
        return fut;;
    }





    std::shared_ptr<internal::remote_callable<Ret, Args...> > _callable;
    int _callable_id;
    execution_pool_pthread* _pool;

    friend class execution_pool_pthread;
    friend struct ::arpc_unit_tests;


};




}; // arpc


}; // mpi


#endif // REMOTE_FUNCTION_HPP

