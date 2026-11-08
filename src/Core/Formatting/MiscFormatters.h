// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <EASTL/string_view.h>

#include "Base.h"

namespace Formatting {

    /// Boolean value formatter.
    template <>
    struct Formatter<bool> {
        constexpr const char* Parse(eastl::string_view spec) { return spec.data(); }
        constexpr void Format(bool value, Writer& writer) const { writer.Write(value ? kTrueString : kFalseString); }

    private:
        static constexpr eastl::string_view kTrueString = "true";
        static constexpr eastl::string_view kFalseString = "false";
    };

    /// Character value formatter.
    template <>
    struct Formatter<char> {
        constexpr const char* Parse(eastl::string_view spec) { return spec.data(); }
        constexpr void Format(char value, Writer& writer) const { writer.Write(value); }
    };

} // namespace Formatting
