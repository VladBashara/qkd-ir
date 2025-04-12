add_library(decoders STATIC decoders.cpp)
target_link_libraries(decoders PUBLIC Eigen3::Eigen ldpc-utils)
target_include_directories(decoders PUBLIC .)