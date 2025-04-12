add_library(benchmarks benchmarks.cpp)
target_link_libraries(benchmarks PUBLIC alist decoders file-processor Taskflow)
target_include_directories(benchmarks PUBLIC .)