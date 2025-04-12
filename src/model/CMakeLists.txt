set(TARGET qkdmodel)

add_library(${TARGET})
target_include_directories(${TARGET} PUBLIC include)
target_sources(${TARGET} PUBLIC qkdmodel.cpp)

target_link_libraries(${TARGET} PUBLIC decoders)
