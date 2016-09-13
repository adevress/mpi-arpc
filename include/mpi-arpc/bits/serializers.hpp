#ifndef SERIALIZERS_HPP
#define SERIALIZERS_HPP
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


#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/tuple.hpp>


namespace mpi {

namespace arpc {


namespace internal{

namespace serializer{

    using input_archiver = cereal::PortableBinaryInputArchive;

    using output_archiver = cereal::PortableBinaryOutputArchive;

}



} // internal



} // arpc

} // mpi



#endif // SERIALIZERS_HPP

