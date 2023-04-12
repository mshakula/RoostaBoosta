# cmake/mbed-setup.cmake
#
# Mbed setup script. Initialize Mbed toolchain. Must be included before any
# project definitions to make sure toolchain selection is not overwritten.

cmake_minimum_required(VERSION 3.19.0 FATAL_ERROR) # MbedOS requires >=3.19.0

if(NOT TARGET)
  message(WARNING "No target specified. Defaulting to LPC1768")
  set(TARGET LPC1768)
endif()

if(NOT TOOLCHAIN)
  message(
    WARNING
      "Toolchain not specified. Supported toolchains: GCC_ARM (default), ARM")
  set(TOOLCHAIN GCC_ARM)
endif()

# ======================================================
# Config Mbed tools and OS. This is needed to be done in configure stage for
# Mbed tools to parse configuration specified in mbed_app.json

find_program(mbedtools NAMES "mbed-tools" "mbedtools")
if(NOT mbedtools)
  message(FATAL_ERROR "[mbedtools] mbed-tools not found.")
endif()

execute_process(
  COMMAND ${mbedtools} --version
  RESULT_VARIABLE mbedtools_result
  OUTPUT_VARIABLE mbedtools_output
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT mbedtools_result EQUAL "0" OR mbedtools_output VERSION_LESS 7)
  message(FATAL_ERROR "[mbedtools] mbed-tools version >= 7 required. Found: "
                      "${mbedtools_output}")
endif()

# configure mbed_config.cmake from mbed_app.json.
execute_process(
  COMMAND
    ${mbedtools} configure -t ${TOOLCHAIN} -m ${TARGET} --output-dir
    ${CMAKE_CURRENT_BINARY_DIR} --program-path ${CMAKE_CURRENT_SOURCE_DIR}
    --mbed-os-path ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mbed-os
  RESULT_VARIABLE mbedtools_result)
if(NOT mbedtools_result EQUAL 0)
  message(FATAL_ERROR "[mbedtools] configure failed.")
endif()

set(MBED_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mbed-os
    CACHE INTERNAL "")
set(MBED_CONFIG_PATH
    ${CMAKE_CURRENT_BINARY_DIR}
    CACHE INTERNAL "")
include(${MBED_PATH}/tools/cmake/app.cmake)
