set(SOURCE_FILES LogSender.cpp LogReceiver.cpp)
set(HEADER_FILES LogSender.h LogReceiver.h)
set(SOURCE_LIST ${SOURCE_FILES} ${HEADER_FILES})

add_library(log-sender STATIC ${SOURCE_LIST})
target_include_directories(log-sender PUBLIC .)

# Подключение библиотеки ws2_32 для Windows
if(WIN32)
    target_link_libraries(log-sender ws2_32)
endif()
