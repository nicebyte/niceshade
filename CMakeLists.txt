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
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libniceshade)

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