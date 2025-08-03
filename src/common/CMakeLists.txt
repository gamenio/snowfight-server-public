
CollectSourceFiles(
	${CMAKE_CURRENT_SOURCE_DIR} 
	COLLECTED_SOURCES)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(common
  ${COLLECTED_SOURCES}
)

target_link_libraries(common
	PUBLIC
    boost
    cppformat
    utf8cpp)
	
target_include_directories(common
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})