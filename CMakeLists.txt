project(mpi-arpc)
cmake_minimum_required(VERSION 2.8)

enable_testing()

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")

option(UNIT_TESTS "Enable or disable unit tests execution" TRUE)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
      ${PROJECT_SOURCE_DIR}/CMake/portability
      ${PROJECT_SOURCE_DIR}/CMake/packages
      ${PROJECT_SOURCE_DIR}/CMake/config)

include(BlueGenePortability)
include(DefineInstallationPaths)
include(ReleaseDebugAutoFlags)

find_package(MPI)

add_definitions( -std=c++11 )


file(GLOB MPI_CPP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/mpi-arpc/*)
file(GLOB MPI_CPP_HEADERS_1 ${CMAKE_CURRENT_SOURCE_DIR}/include/mpi-arpc/*/*)


include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${BOOST_INCLUDE_DIR} )


include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/deps/cereal/include )


add_subdirectory(deps)
add_subdirectory(test/unit)

