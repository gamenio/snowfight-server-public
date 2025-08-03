set(libprotobuf_lite_files
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/arena.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/arenastring.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/extension_set.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/generated_message_table_driven_lite.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/generated_message_util.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/io/coded_stream.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/io/zero_copy_stream.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/io/zero_copy_stream_impl_lite.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/message_lite.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/repeated_field.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/bytestream.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/common.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/int128.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/io_win32.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/once.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/status.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/statusor.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/stringpiece.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/stringprintf.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/structurally_valid.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/strutil.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/time.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/wire_format_lite.cc
)

set(libprotobuf_lite_includes
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/arena.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/arenastring.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/extension_set.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/generated_message_util.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/io/coded_stream.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/io/zero_copy_stream.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/io/zero_copy_stream_impl_lite.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/message_lite.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/repeated_field.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/atomicops_internals_x86_msvc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/bytestream.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/int128.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/once.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/status.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/statusor.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/stringpiece.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/stringprintf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/strutil.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/stubs/time.h
  ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/wire_format_lite.h
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4244 /wd4800 /wd4018")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4800 /wd4018")
endif()

add_library(protobuf-lite STATIC ${libprotobuf_lite_files} ${libprotobuf_lite_includes})

target_include_directories(protobuf-lite
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(protobuf-lite
    PROPERTIES
    FOLDER
    "dep")
