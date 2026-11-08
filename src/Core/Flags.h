// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <type_traits>

#include "Types.h"

/// \def DEFINE_FLAG_OPS
/// Defines bitwise operators and utility functions for a given enum type.
#define DEFINE_FLAG_OPS(TYPE) \
    constexpr bool operator!(TYPE value) \
    { \
        return !std::underlying_type_t<TYPE>(value); \
    } \
    constexpr TYPE operator~(TYPE value) \
    { \
        return TYPE(~std::underlying_type_t<TYPE>(value)); \
    } \
    constexpr TYPE operator&(TYPE lhs, TYPE rhs) \
    { \
        return TYPE(std::underlying_type_t<TYPE>(lhs) & std::underlying_type_t<TYPE>(rhs)); \
    } \
    constexpr TYPE operator|(TYPE lhs, TYPE rhs) \
    { \
        return TYPE(std::underlying_type_t<TYPE>(lhs) | std::underlying_type_t<TYPE>(rhs)); \
    } \
    constexpr TYPE operator^(TYPE lhs, TYPE rhs) \
    { \
        return TYPE(std::underlying_type_t<TYPE>(lhs) ^ std::underlying_type_t<TYPE>(rhs)); \
    } \
    constexpr TYPE& operator&=(TYPE& lhs, TYPE rhs) \
    { \
        return lhs = lhs & rhs; \
    } \
    constexpr TYPE& operator|=(TYPE& lhs, TYPE rhs) \
    { \
        return lhs = lhs | rhs; \
    } \
    constexpr TYPE& operator^=(TYPE& lhs, TYPE rhs) \
    { \
        return lhs = lhs ^ rhs; \
    } \
    constexpr bool Any(TYPE value) \
    { \
        return std::underlying_type_t<TYPE>(value) != 0; \
    } \
    constexpr bool Any(TYPE value, TYPE mask) \
    { \
        return Any(value & mask); \
    } \
    constexpr bool All(TYPE value, TYPE mask) \
    { \
        return (value & mask) == mask; \
    }
