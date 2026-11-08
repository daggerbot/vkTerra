# Copyright (c) 2026, vkTerra Contributors
# SPDX-License-Identifier: MPL-2.0
# See LICENSE-MPL-2.0.txt for more information.

set(EASTL_VERSION 3.27.01)
set(EASTL_URL "https://github.com/electronicarts/EASTL/archive/refs/tags/${EASTL_VERSION}.tar.gz")
set(EASTL_URL_HASH SHA256=fce43bf443f5569b00a8deae735394ea0b16f6c3f96867a17ded50775ffcdd12)

set(PREFIX "${CMAKE_CURRENT_BINARY_DIR}/_install")
set(INCLUDE_DIR "${PREFIX}/include")
set(LIBRARY_NAME EASTL)
set(LIBRARY "${PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")

get_target_property(EABASE_INCLUDE_DIR EABase INTERFACE_INCLUDE_DIRECTORIES)

# Must exist at config time.
file(MAKE_DIRECTORY "${INCLUDE_DIR}")

ExternalProject_Add(ExternalProject-EASTL
    URL "${EASTL_URL}"
    URL_HASH "${EASTL_URL_HASH}"
    PATCH_COMMAND "${VENV_PYTHON}" -m patch_ng "${CMAKE_CURRENT_SOURCE_DIR}/no-fetch-content.patch"
    CMAKE_ARGS
        ${COMMON_CMAKE_ARGS}
        "-DCMAKE_INSTALL_PREFIX=${PREFIX}"
        "-DEABASE_INCLUDE_DIR=${EABASE_INCLUDE_DIR}"
        -DEASTL_BUILD_BENCHMARK=OFF
        -DEASTL_BUILD_TESTS=OFF
        -DEASTL_STD_ITERATOR_CATEGORY_ENABLED=ON
    INSTALL_BYPRODUCTS
        "${LIBRARY}"
)
add_dependencies(ExternalProject-EASTL ExternalProject-EABase venv)

add_library(EASTL STATIC IMPORTED GLOBAL)
add_dependencies(EASTL ExternalProject-EASTL)
set_target_properties(EASTL PROPERTIES
    IMPORTED_LOCATION "${LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES EABase
)
