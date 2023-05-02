# CMakeLists.txt
#
# LPC1768 RTC Interrupt library

cmake_minimum_required(VERSION 3.0)

file(GLOB sources CONFIGURE_DEPENDS *.cpp *.c)

add_library(RTC INTERFACE)

target_sources(RTC INTERFACE ${sources})
target_include_directories(RTC INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
