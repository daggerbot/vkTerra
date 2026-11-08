# Copyright (c) 2026, vkTerra Contributors
# SPDX-License-Identifier: MPL-2.0
# See LICENSE-MPL-2.0.txt for more information.

set(EABASE_REPOSITORY "https://github.com/electronicarts/EABase.git")
set(EABASE_TAG 0699a15efdfd20b6cecf02153bfa5663decb653c)

ExternalProject_Add(ExternalProject-EABase
    GIT_REPOSITORY "${EABASE_REPOSITORY}"
    GIT_TAG "${EABASE_TAG}"
    GIT_SUBMODULES ""
    GIT_SHALLOW TRUE
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(ExternalProject-EABase SOURCE_DIR)
set(INCLUDE_DIR "${SOURCE_DIR}/include/Common")

# Must exist at config time.
file(MAKE_DIRECTORY "${INCLUDE_DIR}")

add_library(EABase INTERFACE IMPORTED GLOBAL)
add_dependencies(EABase ExternalProject-EABase)
set_target_properties(EABase PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${INCLUDE_DIR}"
)
