add_library(alist STATIC alist_matrix.cpp)
target_link_libraries(alist PUBLIC Eigen3::Eigen math)
target_include_directories(alist PUBLIC .)