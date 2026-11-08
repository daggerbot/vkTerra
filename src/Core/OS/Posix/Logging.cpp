// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <EASTL/string_view.h>

#include "../../Ascii.h"
#include "../../Logging.h"
#include "../../LoggingUtils.h"
#include "../../ScopeExit.h"

namespace {

    struct PosixLoggerState {
        pthread_mutex_t mutex;
        Logging::FormatMessageFlags flags;
    };

    PosixLoggerState s_posixLoggerState{
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .flags = Logging::FormatMessageFlags::kEndLine,
    };

    bool IsColorTTY()
    {
        static const eastl::string_view kKnownColorTerms[] = {
            // If you use a fairly common terminal that is not listed here, feel free to add it.
            /* clang-format off */
            "linux",
            "rxvt",
            "rxvt-256color",
            "rxvt-unicode",
            "rxvt-unicode-256color",
            "screen",
            "screen-256color",
            "st",
            "tmux",
            "tmux-256color",
            "xterm",
            "xterm-256color",
            /* clang-format on */
        };

        if (!isatty(STDERR_FILENO)) {
            return false;
        }

        char* term = getenv("TERM");
        if (term && term[0]) {
            for (const auto& entry : kKnownColorTerms) {
                if (strcmp(term, entry.data()) == 0) {
                    return true;
                }
            }
        }
        return false;
    }

    bool ShouldEnableAnsiColors()
    {
        char* noColor = getenv("NO_COLOR"); // <https://no-color.org>
        char* forceColor = getenv("FORCE_COLOR"); // <https://force-color.org>
        if (noColor && noColor[0]) {
            return false;
        } else if (forceColor && forceColor[0]) {
            return true;
        } else {
            return IsColorTTY();
        }
    }

} // namespace

void Logging::Internal::InitPlatformLogger()
{
    if (ShouldEnableAnsiColors()) {
        s_posixLoggerState.flags |= Logging::FormatMessageFlags::kAnsiColors;
    }
}

void Logging::Internal::ShutDownPlatformLogger()
{
}

bool Logging::Internal::PlatformWouldLog(Level)
{
    return true;
}

void Logging::Internal::PlatformLog(Level level, const SourceLocation* location, eastl::string_view fmt,
                                    Formatting::FormatArgs args, const struct ::timespec* time)
{
    MessageBuffer buffer{};
    Logging::FormatMessage(buffer, s_posixLoggerState.flags, level, location, fmt, args, time);

    if (pthread_mutex_lock(&s_posixLoggerState.mutex) != 0) {
        // Lock failed and there's no reliable way to report it.
        return;
    }
    Core::ScopeExit _deferUnlock = [] { pthread_mutex_unlock(&s_posixLoggerState.mutex); };

    const char* message = buffer.data();
    size_t remaining = buffer.size();
    while (remaining) {
        ssize_t writeResult = ::write(STDERR_FILENO, message, remaining);
        if (writeResult <= 0) {
            // Write failed and there's no reliable way to report it.
            break;
        }
        message += static_cast<size_t>(writeResult);
        remaining -= static_cast<size_t>(writeResult);
    }
}
