cmake_minimum_required(VERSION 3.0.2)
project(metadata_parser)

if (WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()

add_library(metadata_parser ${CMAKE_CURRENT_LIST_DIR}/metadata-parser.h ${CMAKE_CURRENT_LIST_DIR}/metadata-parser.c)

set_property(TARGET metadata_parser PROPERTY CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
