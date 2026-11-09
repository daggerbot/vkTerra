# Copyright (c) 2026, vkTerra Contributors
# SPDX-License-Identifier: MPL-2.0
# See LICENSE-MPL-2.0.txt for more information.

#===============================================================================
#
# vkTerraCommon
#
#===============================================================================

add_library(vkTerraCommon INTERFACE)
target_sources(vkTerraCommon INTERFACE
    Core/Error.cpp
    Core/Formatting.cpp
    Core/Logging.cpp
    Core/Memory.cpp
    Core/OS.cpp
    Core/Strings.cpp
)
target_include_directories(vkTerraCommon INTERFACE .)
target_link_libraries(vkTerraCommon INTERFACE
    vkTerraCompilerOptions
    EASTL
)

if(UNIX)
    target_sources(vkTerraCommon INTERFACE
        Core/OS/Posix/Logging.cpp
    )
endif()

#===============================================================================
#
# vkTerraClient
#
#===============================================================================

add_library(vkTerraClient INTERFACE)
target_sources(vkTerraClient INTERFACE
    Client/Main.cpp
    Display/SDL.cpp
    Gpu/GpuSettings.cpp
)
target_compile_definitions(vkTerraClient INTERFACE CLIENT=1)
target_link_libraries(vkTerraClient INTERFACE
    vkTerraCommon
    SDL3
)

#===============================================================================
#
# vkTerra (Vulkan client)
#
#===============================================================================

add_executable(vkTerra WIN32
    Gpu/Vulkan/Init.cpp
    Gpu/Vulkan/State.cpp
    Gpu/Vulkan/Utils.cpp
)
target_compile_definitions(vkTerra PRIVATE GPU_VULKAN=1)
target_link_libraries(vkTerra PRIVATE
    vkTerraClient
    volk
)
