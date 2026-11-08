// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <EASTL/string_view.h>

#include "Types.h"

namespace OS {

    OSChar* getenv(const OSChar* name);

    eastl::string_view GetCurrentThreadDebugName();
    void SetCurrentThreadDebugName(eastl::string_view name);

} // namespace OS
