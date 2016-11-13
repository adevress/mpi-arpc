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

namespace arpc {


///
/// \brief The exec_service_mpi class
///
///  main handle for mpi-arpc asynchronous remote execution
///
class exec_service_mpi{
    class pimpl;
public:
    ///
    /// \brief construct an execution service for arpc with the MPI backend
    /// \param argc
    /// \param argv
    ///
    exec_service_mpi(int* argc, char*** argv);

    ///
    /// \brief ~exec_service_mpi
    ///
    virtual ~exec_service_mpi();

    ///
    /// \brief register a new remote_function in this execution service
    ///
    /// A registered function can call and be called from other node
    ///
    template < typename Fun>
    inline void register_function(Fun & fun){
        using namespace std;

        fun._callable_id =  register_function_internal(fun._callable);
        fun._pool = this;
    }

    ///
    /// \param node_id
    /// return true if node_id is the one of the local node
    ///
    bool is_local(int node_id);

    ////
    ///  internal
    void send_request(int rank, int callable_id, const std::vector<char> & args_serialized,
                       std::unique_ptr<internal::result_object> && result_handler);

    ////
    ///  internal
    void send_request(std::vector<int> node_list, int callable_id, const std::vector<char> & args_serialized,
                       std::unique_ptr<internal::result_object> && result_handler);

private:
    std::unique_ptr<pimpl> d_ptr;

    exec_service_mpi(const exec_service_mpi &) = delete;

    int register_function_internal(std::shared_ptr<internal::callable_object>  callable);

    std::shared_ptr<internal::callable_object> resolve_function_internal(const std::string & function_name);
};






}; // arpc


#endif
