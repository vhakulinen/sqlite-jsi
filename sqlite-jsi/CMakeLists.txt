include(ExternalProject)

# TODO: sqlite3 should be configurable.
ExternalProject_Add(sqlite3
    URL https://www.sqlite.org/2023/sqlite-amalgamation-3430000.zip
    URL_HASH SHA3_256=1286856f5dff3b145f02dfe5aa626657c172a415889e5a265dc4dee83a799c03

    PREFIX ${CMAKE_BINARY_DIR}/sqlite3-download
    SOURCE_DIR ${CMAKE_BINARY_DIR}/sqlite3

    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
    UPDATE_COMMAND ""
)

# Make cmake aware that sqlite3.c file might not be available before sqlite3 target.
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/sqlite3/sqlite3.c
    COMMAND ""
    DEPENDS sqlite3
)

add_library(sqlite-jsi
    src/connection.cpp
    src/database.cpp
    src/executor.cpp
    src/promise.cpp
    src/sqlite-jsi.cpp
    src/statement.cpp
    src/utils.cpp
    src/value.cpp
    ${CMAKE_BINARY_DIR}/sqlite3/sqlite3.c
)

target_compile_options(sqlite-jsi PRIVATE -Wall -Werror)

target_include_directories(sqlite-jsi PUBLIC include)

# Include sqlite.
target_include_directories(sqlite-jsi PUBLIC ${CMAKE_BINARY_DIR}/sqlite3)

if(NOT DEFINED JSI_HEADER_DIR)
    message(STATUS "JSI_HEADER_DIR not defined")

    # Include hermes, which brings jsi for us.
    include(${PROJECT_SOURCE_DIR}/deps/Hermes.cmake)

    message(STATUS "Using default JSI_HEADER_DIR")
    set(JSI_HEADER_DIR "${hermes_SOURCE_DIR}/API/jsi")
endif()

message(STATUS "Using JSI headers from ${JSI_HEADER_DIR}")
target_include_directories(sqlite-jsi PUBLIC ${JSI_HEADER_DIR})
