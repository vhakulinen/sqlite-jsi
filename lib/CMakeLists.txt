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
    connection.cpp
    database.cpp
    executor.cpp
    param.cpp
    sqlite-jsi.cpp
    statement.cpp
    utils.cpp
    ${CMAKE_BINARY_DIR}/sqlite3/sqlite3.c
)

target_compile_features(sqlite-jsi PUBLIC cxx_std_17)

# Include sqlite.
target_include_directories(sqlite-jsi PUBLIC ${CMAKE_BINARY_DIR}/sqlite3)

# TODO: jsi/jsi.h should be configurable.
target_include_directories(sqlite-jsi PUBLIC ${CMAKE_INSTALL_PREFIX}/hermes/include)
