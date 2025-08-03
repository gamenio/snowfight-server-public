# Options relative to SQLite and SQLiteC++ functions

# Enable the use of SQLite column metadata and Column::getColumnOriginName() method,
# Require that the sqlite3 library is also compiled with this flag (default under Debian/Ubuntu, but not on Mac OS X).
add_definitions(-DSQLITE_ENABLE_COLUMN_METADATA)

# Enable the user defintion of a assertion_failed() handler (default to false, easier to handler for begginers).
# add_definitions(-DSQLITECPP_ENABLE_ASSERT_HANDLER)

# Force forward declaration of legacy struct sqlite3_value (pre SQLite 3.19)
# add_definitions(-DSQLITE_USE_LEGACY_STRUCT)

## Build the C++ Wrapper ##

# list of sources files of the library
set(SQLITECPP_SRC
 ${CMAKE_CURRENT_SOURCE_DIR}/src/Backup.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/src/Column.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/src/Database.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/src/Exception.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/src/Statement.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/src/Transaction.cpp
)
source_group(src FILES ${SQLITECPP_SRC})


# list of header files of the library
set(SQLITECPP_INC
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/SQLiteCpp.h
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/Assertion.h
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/Backup.h
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/Column.h
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/Database.h
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/Exception.h
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/Statement.h
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/Transaction.h
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/Utils.h
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/VariadicBind.h
 ${CMAKE_CURRENT_SOURCE_DIR}/include/SQLiteCpp/ExecuteMany.h
)
source_group(include FILES ${SQLITECPP_INC})

# All includes are relative to the "include" directory
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# add sources of the wrapper as a "SQLiteCpp" static library
add_library(SQLiteCpp ${SQLITECPP_SRC} ${SQLITECPP_INC})
# make the sqlite3 library part of the interface of the SQLiteCpp wrapper itself (the client app does not need to link to sqlite3)
target_link_libraries(SQLiteCpp PUBLIC sqlite3)

if (UNIX AND NOT APPLE)
	target_link_libraries(SQLiteCpp PRIVATE dl)
endif ()

target_include_directories(SQLiteCpp PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include")

# build the SQLite3 C library (for ease of use/compatibility) versus Linux sqlite3-dev package
add_subdirectory(sqlite3)
target_include_directories(sqlite3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3")
target_include_directories(SQLiteCpp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3")

set_target_properties(SQLiteCpp
    PROPERTIES
    FOLDER
    "dep")
