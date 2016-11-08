

file(GLOB arpc_mpi_src "*_mpi.cpp" )


add_library(arpc_mpi SHARED ${arpc_mpi_src})
target_link_libraries(arpc_mpi ${MPI_LIBRARIES} ${Boost_THREAD_LIBRARIES} ${Boost_SYSTEM_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
