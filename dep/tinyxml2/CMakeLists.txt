add_library(tinyxml2 tinyxml2.cpp tinyxml2.h)

target_compile_definitions(tinyxml2 PUBLIC $<$<CONFIG:Debug>:TINYXML2_DEBUG>)

target_include_directories(tinyxml2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  target_compile_definitions(tinyxml2 PUBLIC -D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

set_target_properties(tinyxml2
    PROPERTIES
    FOLDER
    "dep")
