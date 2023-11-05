include(${PROJECT_SOURCE_DIR}/deps/SQLite.cmake)
message(STATUS "Using sqlite3 from ${sqlite3_SOURCE_DIR}")

add_library(sqlite-jsi
    src/connection.cpp
    src/database.cpp
    src/executor.cpp
    src/promise.cpp
    src/sqlite-jsi.cpp
    src/statement.cpp
    src/utils.cpp
    src/value.cpp
    ${sqlite3_SOURCE_DIR}/sqlite3.c
)

target_compile_options(sqlite-jsi PRIVATE -Wall -Werror)

target_include_directories(sqlite-jsi PUBLIC include)

# Include sqlite.
target_include_directories(sqlite-jsi PUBLIC ${sqlite3_SOURCE_DIR})

if(NOT DEFINED JSI_HEADER_DIR)
    message(STATUS "JSI_HEADER_DIR not defined")

    # Include hermes, which brings jsi for us.
    include(${PROJECT_SOURCE_DIR}/deps/Hermes.cmake)

    message(STATUS "Using default JSI_HEADER_DIR")
    set(JSI_HEADER_DIR "${hermes_SOURCE_DIR}/API/jsi")
endif()

message(STATUS "Using JSI headers from ${JSI_HEADER_DIR}")
target_include_directories(sqlite-jsi PUBLIC ${JSI_HEADER_DIR})
