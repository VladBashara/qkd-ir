# set(Boost_COMPILER -mgw14)
# set(Boost_ARCHITECTURE -x64)

# find_package(Boost REQUIRED mpi system)
# find_package(MPI REQUIRED)

include_directories(include)

add_executable(print-opt-process print-opt-process.cpp result-mt.cpp)
target_link_libraries(print-opt-process benchmarks)

add_library(genetic_algo_utils genetic_algo_utils.cpp)
target_include_directories(genetic_algo_utils PUBLIC include)
target_link_libraries(genetic_algo_utils PUBLIC ldpc-utils file-processor Eigen3::Eigen)

add_library(genetic_algo genetic_algo.cpp result-mt.cpp)
target_include_directories(genetic_algo PUBLIC include)
target_link_libraries(genetic_algo PUBLIC genetic_algo_utils benchmarks)

# add_library(parallel-framework result.cpp worker.cpp manager.cpp base-manager.cpp)
# target_include_directories(parallel-framework PUBLIC ${Boost_INCLUDE_DIRS} include)
# target_link_libraries(parallel-framework PUBLIC Boost::mpi benchmarks MPI::MPI_CXX)

add_library(parallel-framework-mt result-mt.cpp)
target_include_directories(parallel-framework-mt PUBLIC include)
target_link_libraries(parallel-framework-mt PUBLIC benchmarks)