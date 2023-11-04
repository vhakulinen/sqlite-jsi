
# Include hermes.
include(${PROJECT_SOURCE_DIR}/deps/Hermes.cmake)

add_executable(test-cli main.cpp)

add_dependencies(test-cli sqlite-jsi libhermes)

target_link_libraries(test-cli sqlite-jsi libhermes)
