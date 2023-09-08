include(ExternalProject)

ExternalProject_Add(hermes-engine
    GIT_REPOSITORY https://github.com/facebook/hermes.git
    GIT_TAG rn/0.72-stable
    EXCLUDE_FROM_ALL
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}/hermes
    TEST_COMMAND ""
    UPDATE_COMMAND ""
)

add_library(libhermes SHARED IMPORTED)
set_target_properties(
    libhermes PROPERTIES IMPORTED_LOCATION
    ${CMAKE_INSTALL_PREFIX}/hermes/lib/libhermes.so)

add_executable(test-cli main.cpp)

target_include_directories(test-cli PUBLIC ${CMAKE_INSTALL_PREFIX}/hermes/include)

add_dependencies(test-cli sqlite-jsi libhermes)

target_link_libraries(test-cli sqlite-jsi libhermes)
