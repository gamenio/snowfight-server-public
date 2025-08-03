# Enforce compileparameters for corebuilds under GCC
# This to stop a few silly crashes that could have been avoided IF people
# weren't doing some -O3 psychooptimizations etc.
if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-fno-delete-null-pointer-checks)
endif()

if(MSVC)
	add_definitions(-D_WIN32_WINNT=0x0601)
	
	# 指定源文件字符编码
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /source-charset:utf-8")
endif(MSVC)

add_subdirectory(common)
add_subdirectory(authserver)
add_subdirectory(worldserver)
add_subdirectory(ntsserver)