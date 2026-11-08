// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <EASTL/string_view.h>

#include "Ascii.h"
#include "Strings.h"

std::optional<bool> Strings::ParseBoolPermissive(OSStringView str)
{
    static const eastl::string_view kTrueStrings[] = {"1", "enable", "enabled", "on", "t", "true", "y", "yes"};
    static const eastl::string_view kFalseStrings[] = {"0", "disable", "disabled", "f", "false", "n", "no", "off"};

    for (const auto& entry : kTrueStrings) {
        if (Ascii::EqualsIgnoreCase(str, entry)) {
            return true;
        }
    }
    for (const auto& entry : kFalseStrings) {
        if (Ascii::EqualsIgnoreCase(str, entry)) {
            return false;
        }
    }
    return std::nullopt;
}
