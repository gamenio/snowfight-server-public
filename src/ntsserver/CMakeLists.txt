
CollectSourceFiles(
	${CMAKE_CURRENT_SOURCE_DIR} 
	COLLECTED_SOURCES)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_executable (ntsserver ${COLLECTED_SOURCES})

if( UNIX AND NOT APPLE )
	set(SET_LINK_FLAGS "-pthread ${SET_LINK_FLAGS}")
	target_link_libraries(ntsserver PRIVATE rt)
endif()

set_target_properties(ntsserver PROPERTIES LINK_FLAGS "${SET_LINK_FLAGS}")

target_link_libraries(ntsserver
	PUBLIC
	common
	protobuf-lite)

target_include_directories(ntsserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if( WIN32 )
  if ( MSVC )
    add_custom_command(TARGET ntsserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bin/ntsserver.conf.dist ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
    )
  endif()
endif()

install(TARGETS ntsserver DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(DIRECTORY bin/ DESTINATION "${CMAKE_INSTALL_PREFIX}")
