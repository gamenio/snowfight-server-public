
CollectSourceFiles(
	${CMAKE_CURRENT_SOURCE_DIR} 
	COLLECTED_SOURCES)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_executable (authserver ${COLLECTED_SOURCES})

if( UNIX AND NOT APPLE )
	set(SET_LINK_FLAGS "-pthread ${SET_LINK_FLAGS}")
	target_link_libraries(authserver PRIVATE rt)
endif()

set_target_properties(authserver PROPERTIES LINK_FLAGS "${SET_LINK_FLAGS}")

target_link_libraries(authserver
	PUBLIC
	common
	protobuf-lite
	SQLiteCpp
	maxminddb)

target_include_directories(authserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if( WIN32 )
  if ( MSVC )
    add_custom_command(TARGET authserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/bin/authserver.conf.dist" "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/"
	  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/bin/realmlist.db.sql" "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/"
	  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/bin/bannedlist.db.sql" "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/"
	  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/bin/geoip.mmdb" "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/"
	  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/scripts/"
	  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/bin/scripts/import_auth_sql.bat" "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/scripts/"
    )
  endif()
endif()

install(TARGETS authserver DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(DIRECTORY bin/ DESTINATION "${CMAKE_INSTALL_PREFIX}")
