
CollectSourceFiles(
	${CMAKE_CURRENT_SOURCE_DIR} 
	COLLECTED_SOURCES)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_executable (worldserver ${COLLECTED_SOURCES})

if( UNIX AND NOT APPLE )
  set(SET_LINK_FLAGS "-pthread ${SET_LINK_FLAGS}")
endif()

set_target_properties(worldserver PROPERTIES LINK_FLAGS "${SET_LINK_FLAGS}")

target_link_libraries(worldserver
	PUBLIC
	common
	protobuf-lite
	tinyxml2
	zlib
	SQLiteCpp)
	
if(WIN32)
	target_link_libraries(worldserver
		PUBLIC
		winmm.lib)	
endif()
	
target_include_directories(worldserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
	
#set(CMAKE_DEBUG_TARGET_PROPERTIES
#  INCLUDE_DIRECTORIES
#  COMPILE_DEFINITIONS
#  POSITION_INDEPENDENT_CODE
#  CONTAINER_SIZE_REQUIRED
#  LIB_VERSION
#)

if( WIN32 )
  if ( MSVC )
    add_custom_command(TARGET worldserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bin/worldserver.conf.dist ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
    )
  endif()
endif()

install(TARGETS worldserver DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(DIRECTORY bin/ DESTINATION "${CMAKE_INSTALL_PREFIX}")
