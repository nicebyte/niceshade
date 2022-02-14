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
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/metadata-parser)

add_library(
  dxc-headers INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include/dxc/dxcapi.h
  ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include/dxc/WinAdapter.h)

target_include_directories(
    dxc-headers INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include)

add_library(
  libniceshade
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/technique-parser.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/technique-parser.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/pipeline-layout.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/pipeline-layout-builder.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/pipeline-layout-builder.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/metadata-file-writer.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/platform.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/dynamic-library.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/dxc-wrapper.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/dxc-wrapper.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/separate-to-combined-map.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/separate-to-combined-builder.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/separate-to-combined-builder.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/compilation.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/compilation.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/target.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/instance.cpp
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/metadata-file-writer.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/error.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/pipeline-layout.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/common-types.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/separate-to-combined-map.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/span.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/input.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/output.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/target.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/technique.h
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/instance.h)

target_include_directories(
  libniceshade PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include
  INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include)

target_link_libraries(libniceshade
    dxc-headers metadata-parser spirv-cross-core spirv-cross-reflect)

set(NICEGRAF_SHADERC_SOURCES
	  ${CMAKE_CURRENT_LIST_DIR}/header_file_writer.h
    ${CMAKE_CURRENT_LIST_DIR}/nicegraf_shaderc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/file_utils.h 
    ${CMAKE_CURRENT_LIST_DIR}/file_utils.cpp)

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
target_link_libraries(display_metadata PRIVATE metadata-parser)
set_output_dir(display_metadata ${CMAKE_CURRENT_LIST_DIR}/samples)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
                     
set_target_properties(spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl 
                      spirv-cross spirv-cross-cpp spirv-cross-hlsl spirv-cross-util PROPERTIES FOLDER spirv-cross)
