enable_testing()

add_executable(test-find-intersection test_find_intersection.cpp)
target_link_libraries(test-find-intersection PUBLIC doctest benchmarks)
add_test(NAME test-find-intersection COMMAND test-find-intersection --force-colors -d)

add_executable(test-genetic-algo-utils test-genetic-algo-utils.cpp)
target_link_libraries(test-genetic-algo-utils PUBLIC genetic_algo_utils doctest)
add_test(NAME test-genetic-algo-utils COMMAND test-genetic-algo-utils --force-colors -d)

add_executable(test-genetic-algo test-genetic-algo.cpp)
target_link_libraries(test-genetic-algo PUBLIC genetic_algo doctest)
add_test(NAME test-genetic-algo COMMAND test-genetic-algo --force-colors -d)

add_compile_definitions(CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}")
