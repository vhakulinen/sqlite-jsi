include(FetchContent)

FetchContent_Declare(
    sqlite3
    URL https://www.sqlite.org/2023/sqlite-amalgamation-3430000.zip
    URL_HASH SHA3_256=1286856f5dff3b145f02dfe5aa626657c172a415889e5a265dc4dee83a799c03
)

FetchContent_GetProperties(sqlite3)
if(NOT sqlite3_POPULATED)
    FetchContent_Populate(sqlite3)
endif()
