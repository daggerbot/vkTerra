# Copyright (c) 2026, vkTerra Contributors
# SPDX-License-Identifier: MPL-2.0
# See LICENSE-MPL-2.0.txt for more information.

set(VOLK_VERSION 1.4.350)
set(VOLK_URL "https://github.com/zeux/volk/archive/refs/tags/${VOLK_VERSION}.tar.gz")
set(VOLK_URL_HASH SHA256=e47c6efe5294bb03729a976b385864bba5daf46ec60f0bdea11e1d1446345f9a)

set(PREFIX "${CMAKE_CURRENT_BINARY_DIR}/_install")
set(INCLUDE_DIR "${PREFIX}/include")
set(LIBRARY_NAME volk)
set(LIBRARY "${PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")

# Must exist at config time.
file(MAKE_DIRECTORY "${INCLUDE_DIR}")

ExternalProject_Add(ExternalProject-volk
    URL "${VOLK_URL}"
    URL_HASH "${VOLK_URL_HASH}"
    CMAKE_ARGS
        ${COMMON_CMAKE_ARGS}
        "-DCMAKE_INSTALL_PREFIX=${PREFIX}"
        -DVOLK_HEADERS_ONLY=OFF
        -DVOLK_INSTALL=ON
        -DVOLK_NAMESPACE=OFF
        -DVOLK_PULL_IN_VULKAN=ON
        "-DVULKAN_HEADERS_INSTALL_DIR=${VULKAN_HEADERS_INSTALL_DIR}"
    INSTALL_BYPRODUCTS
        "${LIBRARY}"
)
add_dependencies(ExternalProject-volk ExternalProject-vulkan-headers)

add_library(volk STATIC IMPORTED GLOBAL)
add_dependencies(volk ExternalProject-volk)
set_target_properties(volk PROPERTIES
    IMPORTED_LOCATION "${LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES vulkan-headers
)
