set(GCC_EXPECTED_VERSION 4.7.2)

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS GCC_EXPECTED_VERSION)
  message(FATAL_ERROR "GCC: Project requires version ${GCC_EXPECTED_VERSION} to build but found ${CMAKE_CXX_COMPILER_VERSION}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
message(STATUS "GCC: Enabled c++11 support")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
message(STATUS "GCC: Enabled C99 support")