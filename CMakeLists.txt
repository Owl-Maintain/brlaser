cmake_minimum_required(VERSION 3.10)
project(Owl-Maintain/brlaser CXX)
set(BRLASER_VERSION "6.2.8")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to RelWithDebInfo")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type (default RelWithDebInfo)" FORCE)
endif()

include(CMakePushCheckState)
include(CheckCXXCompilerFlag)
include(CheckIncludeFileCXX)


## Enable assertions for all builds
## (cmake by default sets NDEBUG for release builds)

foreach(var
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
    CMAKE_CXX_FLAGS_MINSIZEREL)
  string(REGEX REPLACE "(^| )[/-]D *NDEBUG($| )" " " "${var}" "${${var}}")
endforeach()


## Configure the compiler

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

macro(extra_cxx_compiler_flag FLAG)
  string(REGEX REPLACE "[^A-Za-z_0-9]" "_" SFLAG ${FLAG})
  check_cxx_compiler_flag(${FLAG} COMPILER_SUPPORT_${SFLAG})
  if(COMPILER_SUPPORT_${SFLAG})
    set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} ${FLAG}")
  endif()
endmacro(extra_cxx_compiler_flag)

# Compiler warnings
extra_cxx_compiler_flag("-Wall")
extra_cxx_compiler_flag("-Wno-missing-braces")
extra_cxx_compiler_flag("-Wdate-time")
# Some hardening flags
extra_cxx_compiler_flag("-fstack-protector-strong")
extra_cxx_compiler_flag("-Wformat")
extra_cxx_compiler_flag("-Wformat -Werror=format-security")
extra_cxx_compiler_flag("-D_FORTIFY_SOURCE=2")
# Enable the supported flags, but give priority to CXXFLAGS env var
set(CMAKE_CXX_FLAGS "${EXTRA_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")


## Configure CUPS

find_program(CUPS_CONFIG NAMES cups-config)
if(NOT CUPS_CONFIG)
  message(FATAL_ERROR "cups-config command not found. Are the CUPS development packages installed?")
endif()

if(NOT CUPS_DATA_DIR)
  execute_process(
    COMMAND "${CUPS_CONFIG}" --datadir
    OUTPUT_VARIABLE CUPS_DATA_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(NOT CUPS_SERVER_BIN)
  execute_process(
    COMMAND "${CUPS_CONFIG}" --serverbin
    OUTPUT_VARIABLE CUPS_SERVER_BIN
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(NOT CUPS_CFLAGS)
  execute_process(
    COMMAND "${CUPS_CONFIG}" --cflags
    OUTPUT_VARIABLE CUPS_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(NOT CUPS_LDFLAGS)
  execute_process(
    COMMAND "${CUPS_CONFIG}" --ldflags
    OUTPUT_VARIABLE CUPS_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(NOT CUPS_LIBS)
  execute_process(
    COMMAND "${CUPS_CONFIG}" --image --libs
    OUTPUT_VARIABLE CUPS_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

cmake_push_check_state()
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CUPS_CFLAGS}")
check_include_file_cxx(cups/raster.h HAVE_CUPS_RASTER_H)
cmake_pop_check_state()
if(NOT HAVE_CUPS_RASTER_H)
  message(SEND_ERROR "<cups/raster.h> header not found. Are the CUPS development packages installed?")
endif()


## Configure paths

if(APPLE)
  set(_DEFAULT_PPD_DIR "/Library/Printers/PPDs/Contents/Resources")
  set(_DEFAULT_FILTER_DIR "/Library/Printers/brlaser/Filter")
else()
  set(_DEFAULT_DRV_DIR "${CUPS_DATA_DIR}/drv")
  set(_DEFAULT_PPD_DIR "${CUPS_DATA_DIR}/model/brlaser")
  set(_DEFAULT_FILTER_DIR "${CUPS_SERVER_BIN}/filter")

  set(CUPS_DRV_DIR "${_DEFAULT_DRV_DIR}" CACHE PATH
    "CUPS drv files directory")
endif()

set(CUPS_PPD_DIR "${_DEFAULT_PPD_DIR}" CACHE PATH
    "CUPS PPD files directory")
set(CUPS_FILTER_DIR "${_DEFAULT_FILTER_DIR}" CACHE PATH
    "CUPS filters directory")


## Build options

option(INSTALL_PPD "Compile and install PPD printer drivers" OFF)

if(APPLE AND NOT INSTALL_PPD)
  set(INSTALL_PPD ON CACHE BOOL "Compile and install PPD printer drivers" FORCE)
  message(STATUS "PPD generation is required on macOS; enabling it automatically")
endif()

option(INSTALL_STAGED "Create a staged installation tree" OFF)

if(INSTALL_STAGED AND NOT INSTALL_PPD)
  set(INSTALL_PPD ON CACHE BOOL "Compile and install PPD printer drivers" FORCE)
  message(STATUS "PPD generation is required on staging; enabling it automatically")
  message(STATUS "Staging installs include both PPD and DRV printer drivers")
endif()


## Build instructions

configure_file(
  "${PROJECT_SOURCE_DIR}/brlaser.drv.in"
  "${PROJECT_BINARY_DIR}/brlaser.drv")

if(INSTALL_PPD)
  find_program(PPDC_EXECUTABLE NAMES ppdc)
  if(NOT PPDC_EXECUTABLE)
    message(FATAL_ERROR "ppdc command not found. Did you try installing it via Homebrew (cups) or MacPorts (cups-ppdc).")
  endif()

  set(BRLASER_DRV "${PROJECT_BINARY_DIR}/brlaser.drv")
  set(BRLASER_PPD_DIR "${PROJECT_BINARY_DIR}/ppd")
  set(BRLASER_PPD_STAMP "${BRLASER_PPD_DIR}/.stamp")

  if(APPLE)
    set(BRLASER_DRV_MACOS "${PROJECT_BINARY_DIR}/brlaser.macos.drv")

    add_custom_command(
      OUTPUT "${BRLASER_DRV_MACOS}"
      COMMAND "${CMAKE_COMMAND}"
        -DINPUT=${BRLASER_DRV}
        -DOUTPUT=${BRLASER_DRV_MACOS}
        -DFILTER_PATH=${CUPS_FILTER_DIR}/rastertobrlaser
        -P "${PROJECT_SOURCE_DIR}/macos-drv.cmake"
      DEPENDS
        "${PROJECT_BINARY_DIR}/brlaser.drv"
        "${PROJECT_SOURCE_DIR}/macos-drv.cmake"
      VERBATIM
      COMMENT "Preparing macOS brlaser driver definition")

    set(BRLASER_DRV "${BRLASER_DRV_MACOS}")
  endif()

  add_custom_command(
    OUTPUT "${BRLASER_PPD_STAMP}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BRLASER_PPD_DIR}"
    COMMAND "${PPDC_EXECUTABLE}" "${BRLASER_DRV}" -d "${BRLASER_PPD_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${BRLASER_PPD_STAMP}"
    DEPENDS "${BRLASER_DRV}"
    VERBATIM
    COMMENT "Generating PPD files with ppdc")

  add_custom_target(brlaser-ppd ALL DEPENDS "${BRLASER_PPD_STAMP}")
endif()

configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h")

include_directories("${PROJECT_BINARY_DIR}")

if(APPLE)
  set(BRLASER_STATE_DIR "/Library/Application Support/brlaser" CACHE STRING "Directory for supply baselines")
else()
  set(BRLASER_STATE_DIR "/var/lib/brlaser" CACHE STRING "Directory for supply baselines")
endif()
add_compile_definitions(BRLASER_STATE_DIR="${BRLASER_STATE_DIR}")

add_executable(commandtobrlaser src/command.cc src/supplies.cc src/status.cc)
target_link_libraries(commandtobrlaser ${CUPS_LIBS})
install(TARGETS commandtobrlaser DESTINATION ${CUPS_SERVER_BIN}/filter)

add_executable(rastertobrlaser src/main.cc src/job.cc src/line.cc src/debug.cc src/status.cc src/supplies.cc src/halftone.cc src/params.cc)
target_compile_options(rastertobrlaser PRIVATE ${CUPS_CFLAGS})
target_link_libraries(rastertobrlaser ${CUPS_LIBS})
target_link_libraries(rastertobrlaser ${CUPS_LDFLAGS})

add_executable(brdecode EXCLUDE_FROM_ALL src/brdecode.cc)
add_executable(test_lest test/test_lest.cc)
add_executable(test_line test/test_line.cc src/line.cc)
add_executable(test_block test/test_block.cc)
add_executable(test_job test/test_job.cc src/job.cc src/line.cc src/status.cc src/supplies.cc)
target_link_libraries(test_job ${CUPS_LIBS})
target_link_libraries(test_job ${CUPS_LDFLAGS})
add_executable(test_line16 test/test_line16.cc src/line.cc)
add_executable(test_halftone test/test_halftone.cc src/halftone.cc)
add_executable(test_supplies test/test_supplies.cc src/supplies.cc)
add_executable(test_status test/test_status.cc src/status.cc src/supplies.cc)
target_link_libraries(test_status ${CUPS_LIBS})
target_link_libraries(test_status ${CUPS_LDFLAGS})
add_executable(test_params test/test_params.cc src/params.cc)
target_link_libraries(test_params ${CUPS_LIBS})
target_link_libraries(test_params ${CUPS_LDFLAGS})

enable_testing()
add_test(test_lest test_lest)
add_test(test_line test_line)
add_test(test_block test_block)
add_test(test_job test_job)
add_test(test_line16 test_line16)
add_test(test_halftone test_halftone)
add_test(test_supplies test_supplies)
add_test(test_status test_status)
add_test(test_params test_params)

# Autotools-style "make check" command
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
add_dependencies(check test_lest test_line test_block test_job test_line16 test_halftone test_supplies test_status test_params)


# Installation

if(INSTALL_STAGED)
  # Staged installs are used to assemble the files that
  # will be packaged into an archive.
  # The paths are configured here because CUPS_FILTER_DIR
  # is used earlier to generate the macOS driver definition.
  set(_STAGING_ROOT "${PROJECT_BINARY_DIR}/staging")
  set(CUPS_FILTER_DIR "${_STAGING_ROOT}")
  set(CUPS_DRV_DIR "${_STAGING_ROOT}")
  set(CUPS_PPD_DIR "${_STAGING_ROOT}/ppd")
endif()

install(
  TARGETS rastertobrlaser
  DESTINATION "${CUPS_FILTER_DIR}")

if(NOT INSTALL_PPD OR INSTALL_STAGED)
  install(
    FILES "${PROJECT_BINARY_DIR}/brlaser.drv"
    DESTINATION "${CUPS_DRV_DIR}")
endif()

if(INSTALL_PPD OR INSTALL_STAGED)
  install(
    DIRECTORY "${BRLASER_PPD_DIR}/"
    DESTINATION "${CUPS_PPD_DIR}"
    FILES_MATCHING
    PATTERN "*.ppd")
endif()
