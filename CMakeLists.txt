cmake_minimum_required (VERSION 2.6)
project (snowfight_server)


if(POLICY CMP0054)
	# Only interpret if() arguments as variables or keywords when unquoted - prevents intepreting if (SOME_STRING_VARIABLE MATCHES "MSVC") as if (SOME_STRING_VARIABLE MATCHES "1")
	cmake_policy(SET CMP0054 NEW)
endif()

if(POLICY CMP0074)
	# Ignore <PackageName>_ROOT variables
	cmake_policy(SET CMP0074 OLD)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

# If no build type is set, the default is “RELEASE” mode
if( NOT CMAKE_BUILD_TYPE )
  set(CMAKE_BUILD_TYPE "RELEASE")
endif()

# Set the default build options
include(cmake/options.cmake)

# Load platform-specific configuration files
include(CheckPlatform)

include(GroupSources)
include(CollectSources)

# Print option configuration results
include(cmake/showoptions.cmake)

add_subdirectory(dep)

add_subdirectory(src)

