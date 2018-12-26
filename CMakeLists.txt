cmake_minimum_required(VERSION 3.0.2)
project(nicegraf_shaderc)

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

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/shaderc)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SPIRV-Cross)

set(NICEGRAF_SHADERC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/nicegraf_shaderc.cpp)

add_executable(nicegraf_shaderc ${NICEGRAF_SHADERC_SOURCES})
target_include_directories(nicegraf_shaderc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../library/include)
target_link_libraries(nicegraf_shaderc
  shaderc spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl)
set_output_dir(nicegraf_shaderc ${CMAKE_CURRENT_LIST_DIR})  
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_library(metadata_parser metadata_parser.h metadata_parser.c)
add_executable(display_metadata ${CMAKE_CURRENT_LIST_DIR}/samples/display_metadata.c)
target_include_directories(display_metadata PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(display_metadata PRIVATE metadata_parser)
set_target_properties(shaderc  glslc glslc_exe install-headers 
                      shaderc_combined_genfile shaderc_shared shaderc_util shaderc-online-compile                      
                      SPIRV-Headers-example SPIRV-Headers-example-1.1
                      spirv-tools-pkg-config spirv-tools-shared-pkg-config
                      PROPERTIES FOLDER shaderc)
                      
set_target_properties(spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl 
                      add-copyright build-version check-copyright spirv-cross spirv-cross-cpp
                      spirv-cross-hlsl spirv-cross-util testdata PROPERTIES FOLDER spirv-cross)