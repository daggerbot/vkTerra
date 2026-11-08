// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <EASTL/string_view.h>

#include "Ascii.h"
#include "Strings.h"

size_t Strings::strlcpy(char* RESTRICT dst, const char* RESTRICT src, size_t dstSize)
{
    size_t i = 0;
    if (dstSize) {
        while (i < dstSize - 1 && src[i]) {
            dst[i] = src[i];
            i++;
        }
        dst[i] = 0;
    }
    while (src[i]) {
        i++;
    }
    return i;
}

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
