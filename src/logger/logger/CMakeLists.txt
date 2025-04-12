set(HEADER_FILES
    logger.h
    loggerDB.h
    datetime.h
)

set(SOURCE_FILES
    logger-main.cpp
    logger.cpp
    datetime.cpp
)

add_executable(logger ${SOURCE_FILES})
target_link_libraries(logger PUBLIC cxxopts)

add_library(logger-lib logger.cpp)
target_link_libraries(logger-lib PUBLIC dp-utils)

target_include_directories(logger-lib PUBLIC include)
target_link_libraries(logger PRIVATE sqlite3)
target_link_libraries(logger-lib PRIVATE sqlite3)

if(WIN32)
    target_link_libraries(logger PRIVATE ws2_32)
    target_link_libraries(logger-lib PRIVATE ws2_32)
endif()
