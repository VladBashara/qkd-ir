configure_file(data/BG2.alist ${CMAKE_CURRENT_BINARY_DIR}/data/BG2.alist COPYONLY)

include_directories(math)

add_subdirectory(LDPC)
add_subdirectory(math)
add_subdirectory(alist)
add_subdirectory(benchmarks)
add_subdirectory(file-processor)
