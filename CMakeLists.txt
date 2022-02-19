cmake_minimum_required(VERSION 3.8)
project(niceshade)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

if (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/build-utils.cmake")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/metadata-parser)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SPIRV-Cross)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)                     
set_target_properties(spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl 
                      spirv-cross spirv-cross-cpp spirv-cross-hlsl spirv-cross-util PROPERTIES FOLDER spirv-cross)


nmk_header_library(NAME dxc-headers
                   SRCS ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include/dxc/dxcapi.h
                        ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include/dxc/WinAdapter.h
                   PUB_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include)
                   
nmk_static_library(NAME libniceshade
                   SRCS ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/technique-parser.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/technique-parser.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/pipeline-layout.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/pipeline-layout-builder.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/pipeline-layout-builder.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/platform.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/dynamic-library.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/dxc-wrapper.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/dxc-wrapper.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/separate-to-combined-builder.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/separate-to-combined-builder.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/compilation.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/compilation.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/target.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/impl/instance.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/error.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/pipeline-layout.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/common-types.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/separate-to-combined-map.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/span.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/input.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/output.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/target.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/technique.h
                        ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include/libniceshade/instance.h
                   PVT_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
                                ${CMAKE_CURRENT_LIST_DIR}/third_party/dxc/include
                                ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include
                   PUB_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/libniceshade/include
                   DEPS dxc-headers metadata-parser spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl)


nmk_binary(NAME niceshade
           SRCS ${CMAKE_CURRENT_LIST_DIR}/cli-tool/metadata-file-writer.h
                ${CMAKE_CURRENT_LIST_DIR}/cli-tool/metadata-file-writer.cpp
                ${CMAKE_CURRENT_LIST_DIR}/cli-tool/header-file-writer.h
                ${CMAKE_CURRENT_LIST_DIR}/cli-tool/niceshade.cpp
                ${CMAKE_CURRENT_LIST_DIR}/cli-tool/target-list.h
                ${CMAKE_CURRENT_LIST_DIR}/cli-tool/file-utils.h 
                ${CMAKE_CURRENT_LIST_DIR}/cli-tool/file-utils.cpp
           PVT_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/../library/include
                        ${CMAKE_CURRENT_LIST_DIR}
           DEPS libniceshade "$<IF:$<BOOL:${WIN32}>,,dl>"
           OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})


nmk_binary(NAME display_metadata
           SRCS ${CMAKE_CURRENT_LIST_DIR}/samples/display-metadata.cpp
                ${CMAKE_CURRENT_LIST_DIR}/cli-tool/file-utils.cpp
           DEPS metadata-parser
           PVT_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
           OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/samples)