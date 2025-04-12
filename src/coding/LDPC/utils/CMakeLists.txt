add_library(ldpc-utils ldpc-utils.cpp)
target_link_libraries(ldpc-utils PUBLIC Eigen3::Eigen math)
target_include_directories(ldpc-utils PUBLIC .)
