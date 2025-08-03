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

# 如果没有设置构建类型则默认为"RELEASE"模式
if( NOT CMAKE_BUILD_TYPE )
  set(CMAKE_BUILD_TYPE "RELEASE")
endif()

# 设置默认的构建选项
include(cmake/options.cmake)

# 加载与平台相关的配置文件
include(CheckPlatform)

include(GroupSources)
include(CollectSources)

# 打印选项配置结果
include(cmake/showoptions.cmake)

add_subdirectory(dep)

add_subdirectory(src)

