cmake_minimum_required(VERSION 3.8)
project(niceshade)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

if (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/../build-utils.cmake")

set(SPIRV_CROSS_CLI OFF)
set(SPIRV_CROSS_ENABLE_TESTS OFF)
set(SPIRV_CROSS_ENABLE_CPP OFF)
set(SPIRV_CROSS_ENABLE_HLSL OFF)
set(SPIRV_CROSS_ENABLE_C_API OFF)
set(SPIRV_CROSS_SKIP_INSTALL ON)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/deps/SPIRV-Cross)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)                     
set_target_properties(spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl 
                      spirv-cross-util PROPERTIES FOLDER spirv-cross)

nmk_header_library(NAME dxc-headers
                   SRCS ${CMAKE_CURRENT_LIST_DIR}/../deps/dxc/include/dxc/dxcapi.h
                        ${CMAKE_CURRENT_LIST_DIR}/../deps/dxc/include/dxc/WinAdapter.h
                   PUB_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/../deps/dxc/include)
                   
nmk_static_library(NAME libniceshade
                   SRCS ${CMAKE_CURRENT_LIST_DIR}/impl/technique-parser.h
                        ${CMAKE_CURRENT_LIST_DIR}/impl/technique-parser.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/impl/pipeline-layout-builder.h
                        ${CMAKE_CURRENT_LIST_DIR}/impl/pipeline-layout-builder.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/impl/platform.h
                        ${CMAKE_CURRENT_LIST_DIR}/impl/dynamic-library.h
                        ${CMAKE_CURRENT_LIST_DIR}/impl/dxc-wrapper.h
                        ${CMAKE_CURRENT_LIST_DIR}/impl/dxc-wrapper.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/impl/separate-to-combined-builder.h
                        ${CMAKE_CURRENT_LIST_DIR}/impl/separate-to-combined-builder.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/impl/compilation.h
                        ${CMAKE_CURRENT_LIST_DIR}/impl/error-macros.h
                        ${CMAKE_CURRENT_LIST_DIR}/impl/compilation.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/impl/target.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/impl/instance.cpp
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/niceshade.h
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/error.h
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/pipeline-layout.h
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/common-types.h
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/separate-to-combined-map.h
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/span.h
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/input.h
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/output.h
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/target.h
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/technique.h
                        ${CMAKE_CURRENT_LIST_DIR}/include/libniceshade/instance.h
                   PVT_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
                                ${CMAKE_CURRENT_LIST_DIR}/../deps/dxc/include
                                ${CMAKE_CURRENT_LIST_DIR}/include
                   PUB_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/include
                   DEPS dxc-headers spirv-cross-core spirv-cross-reflect spirv-cross-glsl spirv-cross-msl)