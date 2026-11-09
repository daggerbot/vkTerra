// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <Core/Types.h>

/// 2-dimensional vector.
template <typename T>
struct Vec2 {
    T x, y;
};

using Vec2i = Vec2<i32>;
using Vec2u = Vec2<u32>;
using Vec2f = Vec2<f32>;

/// 3-dimensional vector.
template <typename T>
struct Vec3 {
    T x, y, z;
};

using Vec3i = Vec3<i32>;
using Vec3u = Vec3<u32>;
using Vec3f = Vec3<f32>;

/// 4-dimensional vector.
template <typename T>
struct Vec4 {
    T x, y, z, w;
};

using Vec4i = Vec4<i32>;
using Vec4u = Vec4<u32>;
using Vec4f = Vec4<f32>;
