// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <Core/Types.h>

namespace Gpu {

    enum class PresentMode : u8 {
        kImmediate,
        kVSync,
        kAdaptive,
        kMailbox,
    };

    struct GpuSettings {
        PresentMode presentMode = PresentMode::kVSync;
    };

    extern GpuSettings g_gpuSettings;

} // namespace Gpu
