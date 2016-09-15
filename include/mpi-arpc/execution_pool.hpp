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

#ifndef _MPI_ARPC_EXECUTION_POOL_HPP_
#define _MPI_ARPC_EXECUTION_POOL_HPP_

#include <string>
#include <functional>
#include <vector>
#include <memory>

#include <mpi.h>

#include "bits/remote_callable.hpp"

namespace mpi {

namespace arpc{

///
/// \brief The execution_pool_pthread class
///
///  main handle for mpi-arpc asynchronous remote execution
///
class execution_pool_pthread{
    class pimpl;
public:
    execution_pool_pthread(int* argc, char*** argv);
    virtual ~execution_pool_pthread();

    template < typename Ret, typename... Args >
    inline void register_function(const std::string & function_name,
                           const std::function<Ret(Args...)> & function_object
                           ){
        using namespace std;
        shared_ptr<internal::callable_object> callable =

                static_pointer_cast<internal::callable_object>(
                        make_shared<internal::remote_callable<Ret, Args...> >(function_object)
                    );

        register_function_internal(function_name, std::move(callable));
    }


private:
    std::unique_ptr<pimpl> d_ptr;

    execution_pool_pthread(const execution_pool_pthread &) = delete;

    void register_function_internal(const std::string &, std::shared_ptr<internal::callable_object> && callable);

    std::shared_ptr<internal::callable_object> resolve_function_internal(const std::string & function_name);
};




}; // arpc


}; // mpi


#endif
