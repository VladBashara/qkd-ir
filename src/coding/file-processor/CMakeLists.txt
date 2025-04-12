add_library(file-processor file-processor.cpp)
target_link_libraries(file-processor PUBLIC alist)
target_include_directories(file-processor PUBLIC .)