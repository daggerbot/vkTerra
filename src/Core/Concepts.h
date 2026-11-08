// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <concepts>
#include <type_traits>

#include "Types.h"

namespace Core {

    /// Matches any of the specified `Types`.
    template <typename T, typename... Types>
    concept AnyOf = (std::same_as<T, Types> || ...);

    /// Matches any character type (`char`, `wchar_t`, `char8_t`, `char16_t`, `char32_t`).
    template <typename T>
    concept AnyChar = AnyOf<std::remove_cv_t<T>, char, wchar_t, char8_t, char16_t, char32_t>;

    /// Matches any integral type except `bool`.
    template <typename T>
    concept IntegralNonBool = std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>;

    /// Matches any integral type except character types and `bool`.
    template <typename T>
    concept StrictIntegral = IntegralNonBool<T> && !AnyChar<T>;

    /// Matches types that can be compared for equality using `==` and `!=`.
    template <typename T, typename R = T>
    concept Equatable = requires(T t, R r) {
        { t == r } -> std::convertible_to<bool>;
        { t != r } -> std::convertible_to<bool>;
    };

    /// Matches types that can be compared for ordering using `==`, `!=`, `<`, `<=`, `>`, and `>=`.
    template <typename T, typename R = T>
    concept Comparable = Equatable<T, R> && requires(T t, R r) {
        { t < r } -> std::convertible_to<bool>;
        { t <= r } -> std::convertible_to<bool>;
        { t > r } -> std::convertible_to<bool>;
        { t >= r } -> std::convertible_to<bool>;
    };

} // namespace Core
