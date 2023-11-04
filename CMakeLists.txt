cmake_minimum_required(VERSION 3.22)

project(sqlite-jsi-root VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# Setting the cxx std value through target_compile_features doesn't for add
# the -std flag to compile_commands.json...
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(sqlite-jsi)
add_subdirectory(tools/test-cli EXCLUDE_FROM_ALL)
