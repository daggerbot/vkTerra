// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <EASTL/fixed_string.h>

#include "Flags.h"
#include "Logging.h"

#undef FormatMessage

namespace Logging {

    // Must be at least large enough that out-of-memory messages do not require heap to report.
    using MessageBuffer = eastl::fixed_string<char, 512>;

    enum class FormatMessageFlags : u32 {
        kNone = 0,
        kAnsiColors = 1 << 0,
        kEndLine = 1 << 1,
    };
    DEFINE_FLAG_OPS(FormatMessageFlags)

    /// Formats a log message into the provided buffer.
    void FormatMessage(MessageBuffer& buffer, FormatMessageFlags flags, Level level, const SourceLocation* location,
                       eastl::string_view fmt, Formatting::FormatArgs args, const struct ::timespec* time);

} // namespace Logging
