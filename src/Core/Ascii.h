// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <type_traits>

#include "Concepts.h"
#include "Strings.h"

/// Utilities for handling ASCII-encoded text.
namespace Ascii {

    template <Core::IntegralNonBool CharT>
    constexpr bool IsControlChar(CharT ch) noexcept
    {
        return ch >= 0 && ch <= 0x1F;
    }

    template <Core::IntegralNonBool CharT>
    constexpr bool IsUppercase(CharT ch) noexcept
    {
        return ch >= 'A' && ch <= 'Z';
    }

    template <Core::IntegralNonBool CharT>
    constexpr bool IsLowercase(CharT ch) noexcept
    {
        return ch >= 'a' && ch <= 'z';
    }

    template <Core::IntegralNonBool CharT>
    constexpr CharT ToUppercase(CharT ch) noexcept
    {
        return Ascii::IsLowercase(ch) ? ch & ~static_cast<CharT>(0x20) : ch;
    }

    template <Core::IntegralNonBool CharT>
    constexpr CharT ToLowercase(CharT ch) noexcept
    {
        return Ascii::IsUppercase(ch) ? ch | static_cast<CharT>(0x20) : ch;
    }

    template <Strings::StringViewable LStr, Strings::StringViewable RStr>
    constexpr bool EqualsIgnoreCase(const LStr& lhs, const RStr& rhs) noexcept
    {
        using LChar = Strings::CharType<LStr>;
        using RChar = Strings::CharType<RStr>;
        auto lstr = Strings::ToStringView(lhs);
        auto rstr = Strings::ToStringView(rhs);
        size_t length = lstr.size();
        if (length != rstr.size()) {
            return false;
        }
        for (size_t i = 0; i < length; i++) {
            auto lchar = static_cast<std::make_unsigned_t<LChar>>(lstr[i]);
            auto rchar = static_cast<std::make_unsigned_t<RChar>>(rstr[i]);
            if (Ascii::ToLowercase(lchar) != Ascii::ToLowercase(rchar)) {
                return false;
            }
        }
        return true;
    }

} // namespace Ascii
