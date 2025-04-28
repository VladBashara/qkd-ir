add_executable(model-demo model-main.cpp)
target_link_libraries(model-demo PUBLIC qkdmodel cxxopts)

add_executable(opt-main opt-main.cpp)
target_link_libraries(opt-main PUBLIC parallel-framework)

add_executable(opt-main-mt opt-main-mt.cpp)
target_link_libraries(opt-main-mt PUBLIC parallel-framework-mt)

add_executable(opt-main-genetic opt-main-genetic.cpp)
target_link_libraries(opt-main-genetic PUBLIC genetic_algo cxxopts)

add_executable(opt-main-greedy opt-main-greedy.cpp)
target_link_libraries(opt-main-greedy PUBLIC parallel-framework-mt cxxopts log-sender)

add_executable(logger-main logger-main.cpp)
target_link_libraries(logger-main PUBLIC log-sender)

add_executable(exposed-main exposed-main.cpp)
target_link_libraries(exposed-main PUBLIC error-estimation)
