include(FetchContent)

FetchContent_Declare(
    hermes
    GIT_REPOSITORY https://github.com/facebook/hermes.git
    GIT_TAG rn/0.72-stable
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(hermes)
if(NOT hermes_POPULATED)
    FetchContent_Populate(hermes)

    # NOTE(ville): Adding EXCLUDE_FROM_ALL to the declare call doesn't seem
    # to have the same effect as this.
    add_subdirectory(${hermes_SOURCE_DIR} ${hermes_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
