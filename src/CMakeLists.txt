

file(GLOB mpi_arpc_src "*.cpp" )


add_library(mpi_arpc ${mpi_arpc_src})
target_link_libraries(mpi_arpc ${MPI_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
