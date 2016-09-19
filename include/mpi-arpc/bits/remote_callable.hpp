#ifndef _REMOTE_CALLABLE_HPP_
#define _REMOTE_CALLABLE_HPP_
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

**/


#include <tuple>
#include <functional>
#include <algorithm>
#include <type_traits>
#include <vector>
#include <sstream>

#include <cstdint>

#include "serializers.hpp"

namespace mpi {

namespace arpc {


namespace internal{

namespace {

template<typename Head, typename... Args>
class list_args{
    typedef Head head_type;

    list_args<Args...> tail;
};

template<>
class list_args<void>{

};


template <typename F, typename Ret, typename Tuple, bool Done, int Total, int... N>
struct call_impl
{
  static Ret call(F f, Tuple && t)
  {
      return call_impl<F, Ret, Tuple, Total == 1 + sizeof...(N), Total, N..., sizeof...(N)>::call(f, std::forward<Tuple>(t));
  }
};

template <typename F, typename Ret, typename Tuple, int Total, int... N>
struct call_impl<F, Ret, Tuple, true, Total, N...>
{
  static Ret call(F f, Tuple && t)
  {
      return f(std::get<N>(std::forward<Tuple>(t))...);
  }
};

template <typename Ret, typename F, typename Tuple>
Ret invoke_function(F f, Tuple && t)
{
  typedef typename std::decay<Tuple>::type ttype;
  return call_impl<F, Ret, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
}


}


///
/// @brief interface to any remote callable object
///
class callable_object{
public:

    inline callable_object(){};
    virtual ~callable_object(){};

    ///
    /// function argument serializer
    ///
    template<typename... Args>
    inline std::vector<char> serialize(Args... args){
        typedef typename std::tuple<Args...> type_tuple;

        using namespace serializer;

        std::vector<char> result;

        std::ostringstream oss;

        type_tuple func_arg(args...);

        output_archiver archiver(oss);

        archiver(func_arg);

        std::string res = oss.str();
        result.resize(res.size()+1);
        std::copy_n(res.begin(), res.size()+1, result.begin());

        return result;
    }



    virtual std::vector<char> deserialize_and_call(const std::vector<char> & arguments) = 0;

};


template<typename Ret, typename... Args>
class remote_callable : public callable_object{
public:
    typedef Ret result_type;

    typedef list_args<Args...> args_type_list;

    typedef typename std::tuple<Args...> type_tuple;


    inline remote_callable() : _func() {}
    inline remote_callable(const std::function<Ret(Args...)> & function) : _func(function) {}

    virtual ~remote_callable(){};

    std::vector<char> serialize_result(result_type arg){
        using namespace serializer;

        std::vector<char> result;

        std::ostringstream oss;
        output_archiver archiver(oss);

        archiver(arg);

        std::string res = oss.str();
        std::copy(res.begin(), res.end(), std::back_inserter(result));

        return result;
    }



    virtual std::vector<char> deserialize_and_call(const std::vector<char> & arguments){

        using namespace serializer;

        std::string input_buffer(arguments.data(), arguments.size());
        std::istringstream iss(input_buffer);

        type_tuple func_arg;

        input_archiver archiver(iss);

        archiver(func_arg);


        result_type res_val = call_from_tuple(func_arg);

        return serialize_result(res_val);
    }

    inline result_type call_from_tuple(const type_tuple & func_arg){
        return invoke_function<result_type>(_func, func_arg);
    }

    inline result_type deserialize_result(const std::vector<char> & result_data){
        using namespace serializer;

        result_type result;
        std::string buffer;
        std::copy(result_data.begin(), result_data.end(), std::back_inserter(buffer));

        std::istringstream iss(buffer);


        input_archiver archiver(iss);

        archiver(result);

        return result;
    }

private:
    std::function<Ret(Args...)> _func;

};




} // internal



} // arpc

} // mpi


#endif
