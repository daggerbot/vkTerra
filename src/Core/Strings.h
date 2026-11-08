// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <optional>

#include <EASTL/string.h>
#include <EASTL/string_view.h>

#include "Concepts.h"

using OSString = eastl::basic_string<OSChar>;
using OSStringView = eastl::basic_string_view<OSChar>;

namespace Strings {

    size_t strlcpy(char* RESTRICT dst, const char* RESTRICT src, size_t dstSize);

    template <Core::IntegralNonBool CharT>
    constexpr eastl::basic_string_view<CharT> ToStringView(const char* str) noexcept
    {
        return str ? eastl::basic_string_view<CharT>{str} : eastl::basic_string_view<CharT>{};
    }

    template <Core::IntegralNonBool CharT>
    constexpr eastl::basic_string_view<CharT> ToStringView(eastl::basic_string_view<CharT> str) noexcept
    {
        return str;
    }

    template <Core::IntegralNonBool CharT>
    constexpr eastl::basic_string_view<CharT> ToStringView(const eastl::basic_string<CharT>& str) noexcept
    {
        return {str.data(), str.size()};
    }

    /// Concept that checks if a type can be converted to a string view using `Strings::ToStringView`.
    template <typename T>
    concept StringViewable = requires(T t) { Strings::ToStringView(t); };

    /// Retrieves the character type of a type that can be converted to a string view using `Strings::ToStringView`.
    template <StringViewable T>
    using CharType = decltype(Strings::ToStringView(std::declval<T>()))::value_type;

    /// Parses a boolean value from the given string, allowing multiple representations such as "true", "false", "1",
    /// "0", "yes", "no", and others.
    std::optional<bool> ParseBoolPermissive(OSStringView str);

} // namespace Strings
