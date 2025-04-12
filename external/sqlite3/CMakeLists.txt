set(TARGET sqlite3)

set(LIBSERIAL_SOURCES
        sqlite3.c
)

set(LIBSERIAL_HEADERS
        include/sqlite/sqlite3.h
)

add_library(${TARGET})

target_sources(${TARGET} PRIVATE ${LIBSERIAL_SOURCES})

target_sources(${TARGET} PUBLIC ${LIBSERIAL_HEADERS})

target_include_directories(${TARGET} PUBLIC include)
