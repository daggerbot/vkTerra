// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <EASTL/string.h>
#include <EASTL/string_view.h>

#include "Base.h"

namespace Formatting {

    /// String view formatter.
    template <>
    struct Formatter<eastl::string_view> {
        constexpr const char* Parse(eastl::string_view spec) { return spec.data(); }
        constexpr void Format(eastl::string_view value, Writer& writer) const { writer.Write(value); }

        static constexpr size_t GetLengthBounded(const char* str, size_t maxLength)
        {
            size_t length = 0;
            while (length < maxLength && str[length]) {
                length++;
            }
            return length;
        }

        constexpr void FormatCharArray(const char* value, size_t extent, Writer& writer) const
        {
            Format(eastl::string_view{value, GetLengthBounded(value, extent)}, writer);
        }
    };

    /// C string formatter.
    template <>
    struct Formatter<const char*> : Formatter<eastl::string_view> {
        constexpr void Format(const char* value, Writer& writer) const
        {
            Formatter<eastl::string_view>::Format(value ? eastl::string_view{value} : kNullFallbackString, writer);
        }
    };

    /// Character array formatter.
    template <size_t N>
    struct Formatter<char[N]> : Formatter<eastl::string_view> {
        constexpr void Format(char (&value)[N], Writer& writer) const
        {
            Formatter<eastl::string_view>::FormatCharArray(value, N, writer);
        }
    };

    /// String formatter.
    template <>
    struct Formatter<eastl::string> : Formatter<eastl::string_view> {};

} // namespace Formatting
