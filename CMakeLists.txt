cmake_minimum_required(VERSION 3.10)

# set the project name
project(cracklist VERSION 0.1)

# Set build type default Release
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

add_subdirectory(./SimdHash/)
add_subdirectory(./libdispatchqueue/)

# specify clang
SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message("Detected platform: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
if (NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    if (NOT AVX OR AVX STREQUAL "" OR AVX STREQUAL "256")
        set(AVXFLAG "-mavx2" CACHE STRING "" FORCE)
    elseif(AVX STREQUAL "512")
        set(AVXFLAG "-mavx512bw" CACHE STRING "" FORCE)
    endif()
endif()

# set compiler flags
set(CMAKE_CXX_FLAGS "-Wall -Werror -Wno-deprecated-declarations -Wno-ignored-optimization-argument -Wno-deprecated-declarations ${AVXFLAG}")
set(CMAKE_EXE_LINKER_FLAGS "-licuuc")

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -funroll-loops")
set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb -O0 -fno-omit-frame-pointer -fsanitize=address -DDEBUG")

if(APPLE)
    execute_process(COMMAND brew --prefix OUTPUT_VARIABLE HOMEBREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    message("Homebrew prefix: ${HOMEBREW_PREFIX}")
    set(HOMEBREW_INCLUDE ${HOMEBREW_PREFIX}/include)
    include_directories(${HOMEBREW_PREFIX}/include)
    link_directories(${HOMEBREW_PREFIX}/lib)
else()
    set(HOMEBREW_INCLUDE "")
endif()

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(cracklist ${SOURCES})
set_property(TARGET cracklist PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
target_include_directories(cracklist
                            PUBLIC
                                ./src/
                                ./SimdHash/src/
                        )
target_link_libraries(cracklist simdhash dispatchqueue)
