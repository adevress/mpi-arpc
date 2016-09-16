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

    template < typename Fun>
    inline void register_function(Fun & fun){
        using namespace std;

        fun._callable_id =  register_function_internal(fun._callable);
        fun._pool = this;
    }

    bool is_local(int rank);

    void send_request(int rank, int callable_id, const std::vector<char> & args_serialized,
                      const std::function<void (const std::vector<char> &)> & callback);
private:
    std::unique_ptr<pimpl> d_ptr;

    execution_pool_pthread(const execution_pool_pthread &) = delete;

    int register_function_internal(std::shared_ptr<internal::callable_object>  callable);

    std::shared_ptr<internal::callable_object> resolve_function_internal(const std::string & function_name);
};




}; // arpc


}; // mpi


#endif
