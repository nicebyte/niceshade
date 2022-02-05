cmake_minimum_required(VERSION 3.0.2)
project(niceshade)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

if (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
endif()

function(set_output_dir target dir)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${dir}")
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${dir}")
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${dir}")
endfunction(set_output_dir)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SPIRV-Cross)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/metadata_parser)

add_library(
  dxc-headers INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include/dxc/dxcapi.h
  ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include/dxc/WinAdapter.h)

target_include_directories(
    dxc-headers INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include)

add_library(
  libniceshade
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/common-types.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/technique-parser.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/technique-parser.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/platform.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/dynamic-library.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/dxc-wrapper.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/dxc-wrapper.cpp)

target_include_directories(
  libniceshade PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include)

target_link_libraries(libniceshade
    dxc-headers)

set(NICEGRAF_SHADERC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/compilation.h
    ${CMAKE_CURRENT_LIST_DIR}/compilation.cpp
	  ${CMAKE_CURRENT_LIST_DIR}/header_file_writer.h
    ${CMAKE_CURRENT_LIST_DIR}/nicegraf_shaderc.cpp
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
target_include_directories(nicegraf_shaderc PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/../library/include)
target_link_libraries(nicegraf_shaderc
  libniceshade
  spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl)
if (NOT WIN32)
  target_link_libraries(nicegraf_shaderc dl)
endif()
set_output_dir(nicegraf_shaderc ${CMAKE_CURRENT_LIST_DIR})

add_executable(display_metadata ${CMAKE_CURRENT_LIST_DIR}/samples/display_metadata.cpp ${CMAKE_CURRENT_LIST_DIR}/file_utils.cpp)
target_include_directories(display_metadata PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(display_metadata PRIVATE metadata_parser)
set_output_dir(display_metadata ${CMAKE_CURRENT_LIST_DIR}/samples)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
                     
set_target_properties(spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl 
                      spirv-cross spirv-cross-cpp spirv-cross-hlsl spirv-cross-util PROPERTIES FOLDER spirv-cross)
