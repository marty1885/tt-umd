cmake_minimum_required(VERSION 3.16)
project(umd_device)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(MASTER_PROJECT OFF)
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MASTER_PROJECT ON)
    message("-- UMD: Building as master project")
endif()

if(MASTER_PROJECT)
    if(NOT CMAKE_BUILD_TYPE)
        message(STATUS "Setting build type to 'Release' as none was specified.")
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Release build is the default" FORCE)
    endif()

    add_library(compiler_flags INTERFACE)
    target_compile_options(compiler_flags INTERFACE -DFMT_HEADER_ONLY)
endif()
message(STATUS "UMD build type: ${CMAKE_BUILD_TYPE}")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DDEBUG=DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG=DEBUG")

include(${PROJECT_SOURCE_DIR}/cmake/dependencies.cmake)

add_library(umd_common_directories INTERFACE)
target_include_directories(umd_common_directories INTERFACE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/device ${PROJECT_SOURCE_DIR}/third_party/fmt/include)

if(NOT DEFINED ENV{ARCH_NAME})
    message(FATAL_ERROR "Please set ARCH_NAME to grayskull, wormhole_b0, or blackhole")
elseif($ENV{ARCH_NAME} STREQUAL "grayskull")
    message(STATUS "UMD: Building for Grayskull")
    target_include_directories(umd_common_directories INTERFACE 
        ${PROJECT_SOURCE_DIR}/device/grayskull 
        ${PROJECT_SOURCE_DIR}/src/firmware/riscv/grayskull
    )
elseif($ENV{ARCH_NAME} STREQUAL "wormhole_b0")
    message(STATUS "UMD: Building for Wormhole")
    target_include_directories(umd_common_directories INTERFACE 
        ${PROJECT_SOURCE_DIR}/device/wormhole 
        ${PROJECT_SOURCE_DIR}/src/firmware/riscv/wormhole
    )
elseif($ENV{ARCH_NAME} STREQUAL "blackhole")
    message(STATUS "UMD: Building for Blackhole")
    target_include_directories(umd_common_directories INTERFACE 
        ${PROJECT_SOURCE_DIR}/device/blackhole 
        ${PROJECT_SOURCE_DIR}/src/firmware/riscv/blackhole
    )
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/device)
add_subdirectory(${PROJECT_SOURCE_DIR}/tests EXCLUDE_FROM_ALL)

