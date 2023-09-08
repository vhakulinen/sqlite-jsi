cmake_minimum_required(VERSION 3.22)

project(sqlite-jsi-root VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_compile_options(
    -Wall -Werror -g
)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib)
add_subdirectory(${CMAKE_SOURCE_DIR}/tools/test-cli)
