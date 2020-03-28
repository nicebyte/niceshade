cmake_minimum_required(VERSION 3.0.2)
project(nicegraf_shaderc)

if (WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()

function(set_output_dir target dir)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${dir}")
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${dir}")
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${dir}")
endfunction(set_output_dir)

set(SHADERC_SKIP_TESTS "on")
set(SPIRV_SKIP_TESTS "on")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SPIRV-Cross)

set(NICEGRAF_SHADERC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/dxc_wrapper.h
    ${CMAKE_CURRENT_LIST_DIR}/dxc_wrapper.cpp
	  ${CMAKE_CURRENT_LIST_DIR}/header_file_writer.h
    ${CMAKE_CURRENT_LIST_DIR}/nicegraf_shaderc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/technique_parser.h
    ${CMAKE_CURRENT_LIST_DIR}/technique_parser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/shader_defines.h
    ${CMAKE_CURRENT_LIST_DIR}/file_utils.h 
    ${CMAKE_CURRENT_LIST_DIR}/file_utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pipeline_layout.h
    ${CMAKE_CURRENT_LIST_DIR}/pipeline_layout.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pipeline_metadata_file.h
    ${CMAKE_CURRENT_LIST_DIR}/pipeline_metadata_file.cpp
    ${CMAKE_CURRENT_LIST_DIR}/separate_to_combined_map.h
    ${CMAKE_CURRENT_LIST_DIR}/separate_to_combined_map.cpp)

add_executable(nicegraf_shaderc ${NICEGRAF_SHADERC_SOURCES})
set_property(TARGET nicegraf_shaderc PROPERTY CXX_STANDARD 17)
target_include_directories(nicegraf_shaderc PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/../library/include
    ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc)
target_link_libraries(nicegraf_shaderc
  spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl)
set_output_dir(nicegraf_shaderc ${CMAKE_CURRENT_LIST_DIR})

add_library(metadata_parser metadata_parser/metadata_parser.h metadata_parser/metadata_parser.c)

add_executable(display_metadata ${CMAKE_CURRENT_LIST_DIR}/samples/display_metadata.cpp ${CMAKE_CURRENT_LIST_DIR}/file_utils.cpp)
target_include_directories(display_metadata PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(display_metadata PRIVATE metadata_parser)
set_output_dir(display_metadata ${CMAKE_CURRENT_LIST_DIR}/samples)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
                     
set_target_properties(spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl 
                      spirv-cross spirv-cross-cpp spirv-cross-hlsl spirv-cross-util PROPERTIES FOLDER spirv-cross)
