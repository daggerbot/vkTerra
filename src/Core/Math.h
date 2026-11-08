// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <type_traits>

#include "Concepts.h"

#undef max
#undef min

namespace Math {

    template <typename N, Core::Comparable<N> Min, Core::Comparable<N> Max>
    constexpr std::common_type_t<N, Min, Max> Clamp(N n, Min min, Max max)
    {
        return n < min ? min : (n > max ? max : n);
    }

    template <typename A, Core::Comparable<A> B>
    constexpr std::common_type_t<A, B> Max(A a, B b)
    {
        return b > a ? b : a;
    }

    template <typename A, Core::Comparable<A> B>
    constexpr std::common_type_t<A, B> Min(A a, B b)
    {
        return b < a ? b : a;
    }

} // namespace Math
