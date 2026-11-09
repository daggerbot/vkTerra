// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <Math/Vector.h>

namespace Display {

    void InitDisplay();
    void ShutDownDisplay();
    void PollEvents();
    Vec2u GetSurfaceSizePixels();

} // namespace Display
