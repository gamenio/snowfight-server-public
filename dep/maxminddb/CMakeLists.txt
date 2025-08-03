set(MAXMINDDB_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/data-pool.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/maxminddb.c
)

set(MAXMINDDB_INC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/data-pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/maxminddb.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/maxminddb_config.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/maxminddb-compat-util.h
)

if(NOT MSVC)
	set(MAXMINDDB_INC ${MAXMINDDB_INC}
		${CMAKE_CURRENT_SOURCE_DIR}/src/config.h)
endif()

add_library(maxminddb STATIC ${MAXMINDDB_SRC} ${MAXMINDDB_INC})

target_include_directories(maxminddb
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
	target_compile_definitions(maxminddb PRIVATE -D_UNICODE -DUNICODE)
else()
	target_compile_definitions(maxminddb PRIVATE -DHAVE_CONFIG_H)
endif()

set_target_properties(maxminddb
    PROPERTIES
    FOLDER
    "dep")
