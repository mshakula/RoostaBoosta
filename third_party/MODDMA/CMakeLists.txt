# CMakeLists.txt
#
# MODDMA library

cmake_minimum_required(VERSION 3.0)

file(GLOB sources CONFIGURE_DEPENDS *.cpp *.c)

add_library(MODDMA INTERFACE)

target_sources(MODDMA INTERFACE ${sources})
target_include_directories(MODDMA INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
