// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <concepts>
#include <type_traits>

#include <EASTL/string_view.h>

#include "Types.h"

namespace Unicode {

    inline constexpr char8_t kMaxAsciiChar = 0x7F;
    inline constexpr char16_t kMinSurrogate = 0xD800;
    inline constexpr char16_t kMinHighSurrogate = 0xD800;
    inline constexpr char16_t kMaxHighSurrogate = 0xDBFF;
    inline constexpr char16_t kMinLowSurrogate = 0xDC00;
    inline constexpr char16_t kMaxLowSurrogate = 0xDFFF;
    inline constexpr char16_t kMaxSurrogate = 0xDFFF;
    inline constexpr char16_t kReplacementChar = 0xFFFD;
    inline constexpr char16_t kMaxBmpChar = 0xFFFF;
    inline constexpr char32_t kMaxChar = 0x10FFFF;

    void CallWithUtf8ToWide(eastl::string_view inStr, void (*callback)(void* userData, eastl::wstring_view outStr),
                            void* userData);
    void CallWithWideToUtf8(eastl::wstring_view inStr, void (*callback)(void* userData, eastl::string_view outStr),
                            void* userData);

    template <std::invocable<eastl::wstring_view> Callback>
    void CallWithUtf8ToWide(eastl::string_view inStr, Callback&& callback)
    {
        auto thunk = [](void* userData, eastl::wstring_view outStr) {
            (*static_cast<std::remove_reference_t<Callback>*>(userData))(outStr);
        };
        Unicode::CallWithUtf8ToWide(inStr, thunk, &callback);
    }

    template <std::invocable<eastl::string_view> Callback>
    void CallWithWideToUtf8(eastl::wstring_view inStr, Callback&& callback)
    {
        auto thunk = [](void* userData, eastl::string_view outStr) {
            (*static_cast<std::remove_reference_t<Callback>*>(userData))(outStr);
        };
        Unicode::CallWithWideToUtf8(inStr, thunk, &callback);
    }

} // namespace Unicode
