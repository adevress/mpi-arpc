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

#include "bits/remote_callable.hpp"

namespace mpi {

namespace arpc{

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

    remote_function(const std::function<Ret(Args...)> & function_object){

        using namespace std;
        _callable = make_shared<internal::remote_callable<Ret, Args... > >(function_object);
    }

    virtual ~remote_function(){};




    inline result_type execute_local(Args... args){

        std::vector<char> args_serialized = _callable->serialize(args... );
        std::vector<char> res_serialized = _callable->deserialize_and_call(args_serialized);
        return _callable->deserialize_result(res_serialized);
    }


private:
    remote_function(const remote_function &) = delete;

    std::shared_ptr<internal::remote_callable<Ret, Args...> > _callable;

};




}; // arpc


}; // mpi


#endif // REMOTE_FUNCTION_HPP

