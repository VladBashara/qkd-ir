add_library(error-estimation result-exposed.cpp)
target_include_directories(error-estimation PUBLIC .)
target_link_libraries(error-estimation PUBLIC benchmarks)