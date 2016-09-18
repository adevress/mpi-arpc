project(mpi-arpc)
cmake_minimum_required(VERSION 2.8)

enable_testing()

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")

option(UNIT_TESTS "Enable or disable unit tests compilation" TRUE)
option(PERF_TESTS "Enable or disable perf tests compilation" TRUE)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
      ${PROJECT_SOURCE_DIR}/CMake/portability
      ${PROJECT_SOURCE_DIR}/CMake/packages
      ${PROJECT_SOURCE_DIR}/CMake/config)

include(BlueGenePortability)
include(DefineInstallationPaths)
include(ReleaseDebugAutoFlags)

find_package(Threads)
find_package(Boost 1.41.0 REQUIRED system thread)
find_package(MPI)

add_definitions( -std=c++11 )


file(GLOB MPI_CPP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/mpi-arpc/*)
file(GLOB MPI_CPP_HEADERS_1 ${CMAKE_CURRENT_SOURCE_DIR}/include/mpi-arpc/*/*)


include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include/ )
include_directories(  ${BOOST_INCLUDE_DIR}  )
include_directories(  ${MPI_C_INCLUDE_PATH}  )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/deps/cereal/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/deps/mpi-cpp/include )

add_subdirectory(src)


add_subdirectory(deps)
add_subdirectory(test/unit)
add_subdirectory(test/perf)

