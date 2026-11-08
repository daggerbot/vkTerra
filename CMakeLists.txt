# Copyright (c) 2026, vkTerra Contributors
# SPDX-License-Identifier: MPL-2.0
# See LICENSE-MPL-2.0.txt for more information.

cmake_minimum_required(VERSION 3.31)
project(vkTerra C CXX)

set(BINDIR "${PROJECT_BINARY_DIR}/bin")
set(SRCDIR "${PROJECT_SOURCE_DIR}/src")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BINDIR}")
set(CMAKE_SKIP_RPATH ON)

#===============================================================================
#
# Compiler Options
#
#===============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(vkTerraCompilerOptions INTERFACE)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(vkTerraCompilerOptions INTERFACE $<$<COMPILE_LANGUAGE:CXX>:
        -Wall
        -Wconversion
        -Wextra
        -Wpedantic
        -Wsuggest-override
        "-fmacro-prefix-map=${SRCDIR}/="
        -fno-exceptions
        -fno-rtti
    >)
endif()

if(WIN32)
    target_compile_definitions(vkTerraCompilerOptions INTERFACE
        _UNICODE
        _WIN32_WINNT=0x0A00 # Windows 10
        NOMINMAX
        UNICODE
        WIN32_LEAN_AND_MEAN
    )
endif()

#===============================================================================
#
# Python Virtual Environment
#
#===============================================================================

find_package(Python3 REQUIRED)

set(VENV_DIR_DEFAULT "${PROJECT_BINARY_DIR}/venv")
set(VENV_DIR "${VENV_DIR_DEFAULT}" CACHE PATH "Python virtual environment directory")
set(VENV_BINDIR "${VENV_DIR}/bin")
set(VENV_PYTHON "${VENV_BINDIR}/python3${CMAKE_HOST_EXECUTABLE_SUFFIX}")
set(VENV_REQUIREMENTS "${PROJECT_SOURCE_DIR}/tools/requirements.txt")
set(VENV_STAMP "${VENV_DIR}/venv.stamp")

add_custom_command(
    OUTPUT "${VENV_STAMP}"
    COMMAND "${Python3_EXECUTABLE}" -m venv "${VENV_DIR}"
    COMMAND "${VENV_PYTHON}" -m pip install -U pip
    COMMAND "${VENV_PYTHON}" -m pip install -r "${VENV_REQUIREMENTS}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${VENV_STAMP}"
    DEPENDS "${VENV_REQUIREMENTS}"
    COMMENT "Setting up Python virtual environment"
    USES_TERMINAL)

add_custom_target(venv ALL DEPENDS "${VENV_STAMP}")

#===============================================================================
#
# Build Targets
#
#===============================================================================

add_subdirectory(thirdparty)
add_subdirectory(src)

#===============================================================================
#
# Run Targets
#
#===============================================================================

if(CMAKE_GENERATOR MATCHES "Ninja|Ninja Multi-Config|Unix Makefiles")
    set(RUN_ARGS "" CACHE STRING "Additional arguments when running the game client")
    set(RUN_COMMAND "" CACHE STRING "Command prefix when running the game client")

    if(LINUX OR BSD)
        list(APPEND RUN_COMMAND env "LD_LIBRARY_PATH=${BINDIR}")
    endif()

    add_custom_target(run
        COMMAND ${RUN_COMMAND} "$<TARGET_FILE:vkTerra>" ${RUN_ARGS}
        DEPENDS vkTerra ${DLL_TARGETS}
        USES_TERMINAL)
endif()
