# Copyright (c) 2026, vkTerra Contributors
# SPDX-License-Identifier: MPL-2.0
# See LICENSE-MPL-2.0.txt for more information.

set(VULKAN_HEADERS_REPO "https://github.com/KhronosGroup/Vulkan-Headers.git")
set(VULKAN_HEADERS_TAG 9fbc2a782ee3f8a4db64e7d991e23da75e893f44)

ExternalProject_Add(ExternalProject-vulkan-headers
    GIT_REPOSITORY "${VULKAN_HEADERS_REPO}"
    GIT_TAG "${VULKAN_HEADERS_TAG}"
    GIT_SUBMODULES ""
    GIT_SHALLOW ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(ExternalProject-vulkan-headers SOURCE_DIR)
set(INCLUDE_DIR "${SOURCE_DIR}/include")
set(VULKAN_HEADERS_INSTALL_DIR "${SOURCE_DIR}" PARENT_SCOPE)

# Must exist at config time.
file(MAKE_DIRECTORY "${INCLUDE_DIR}")

add_library(vulkan-headers INTERFACE)
add_dependencies(vulkan-headers ExternalProject-vulkan-headers)
set_target_properties(vulkan-headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${INCLUDE_DIR}"
)
