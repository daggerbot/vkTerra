// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <stdlib.h>
#include <time.h>

#include <GameDefs.h>

#include "Ascii.h"
#include "Logging.h"
#include "LoggingUtils.h"
#include "Math.h"
#include "OS.h"
#include "ScopeExit.h"
#include "Strings.h"

#define ENV_VAR_LOCAL_TIME GAME_ENV_VAR_PREFIX "LOCAL_TIME"
#define ENV_VAR_LOG_LEVEL GAME_ENV_VAR_PREFIX "LOG_LEVEL"
#define ENV_VAR_SOURCE_LOCATION GAME_ENV_VAR_PREFIX "SOURCE_LOCATION"
#define ENV_VAR_TIMESTAMPS GAME_ENV_VAR_PREFIX "TIMESTAMPS"

struct tm;

namespace {

#ifdef NDEBUG
# if CLIENT
    constexpr Logging::Level kDefaultLogLevel = Logging::Level::kWarn;
# else
    constexpr Logging::Level kDefaultLogLevel = Logging::Level::kInfo;
# endif
#else
    constexpr Logging::Level kDefaultLogLevel = Logging::Level::kDebug;
#endif

    struct LoggerState {
        Logging::Level currentLevel = Logging::Level::kNone;
        bool sourceLocationEnabled = false;
        bool timestampsEnabled = false;
        bool localTimeEnabled = false;
    };

    LoggerState s_loggerState{
#ifndef NDEBUG
        .sourceLocationEnabled = true,
#endif
#if DEDICATED_SERVER
        .timestampsEnabled = true,
#endif
    };
    thread_local bool tls_currentlyLogging = false;

} // namespace

//==============================================================================
//
// Public Logging API
//
//==============================================================================

void Logging::InitLogger()
{
    s_loggerState.currentLevel = kDefaultLogLevel;
    Internal::InitPlatformLogger();

    // Check environment setting for log level.
    OSChar* levelStr = OS::getenv(OSSTR(ENV_VAR_LOG_LEVEL));
    if (levelStr && levelStr[0]) {
        std::optional<Level> parsedLevel = Logging::ParseLogLevel(levelStr);
        if (parsedLevel.has_value()) {
            Logging::SetLogLevel(*parsedLevel);
        } else {
            LOG_WARN(ENV_VAR_LOG_LEVEL ": Invalid log level: {}", levelStr);
        }
    }

    // Check environment setting for source location.
    OSChar* sourceLocationStr = OS::getenv(OSSTR(ENV_VAR_SOURCE_LOCATION));
    if (sourceLocationStr && sourceLocationStr[0]) {
        std::optional<bool> parsedSourceLocation = Strings::ParseBoolPermissive(sourceLocationStr);
        if (parsedSourceLocation.has_value()) {
            s_loggerState.sourceLocationEnabled = *parsedSourceLocation;
        } else {
            LOG_WARN(ENV_VAR_SOURCE_LOCATION ": Invalid boolean: {}", sourceLocationStr);
        }
    }

    // Check environment setting for timestamps.
    OSChar* timestampsStr = OS::getenv(OSSTR(ENV_VAR_TIMESTAMPS));
    if (timestampsStr && timestampsStr[0]) {
        std::optional<bool> parsedTimestamps = Strings::ParseBoolPermissive(timestampsStr);
        if (parsedTimestamps.has_value()) {
            s_loggerState.timestampsEnabled = *parsedTimestamps;
        } else {
            LOG_WARN(ENV_VAR_TIMESTAMPS ": Invalid boolean: {}", timestampsStr);
        }
    }

    // Check environment setting for local time.
    OSChar* localTimeStr = OS::getenv(OSSTR(ENV_VAR_LOCAL_TIME));
    if (localTimeStr && localTimeStr[0]) {
        std::optional<bool> parsedLocalTime = Strings::ParseBoolPermissive(localTimeStr);
        if (parsedLocalTime.has_value()) {
            s_loggerState.localTimeEnabled = *parsedLocalTime;
        } else {
            LOG_WARN(ENV_VAR_LOCAL_TIME ": Invalid boolean: {}", localTimeStr);
        }
    }
}

void Logging::ShutDownLogger()
{
    Internal::ShutDownPlatformLogger();
    s_loggerState.currentLevel = Level::kNone;
}

void Logging::SetLogLevel(Level level)
{
    s_loggerState.currentLevel = level;
}

bool Logging::WouldLog(Level level)
{
    return level != Level::kNone && level <= s_loggerState.currentLevel && !tls_currentlyLogging
           && Internal::PlatformWouldLog(level);
}

std::optional<Logging::Level> Logging::ParseLogLevel(OSStringView str)
{
    static const struct {
        eastl::string_view name;
        Level level;
    } kLevelNames[] = {
        /* clang-format off */
        {"none", Level::kNone},
        {"off", Level::kNone},
        {"fatal", Level::kFatal},
        {"error", Level::kError},
        {"warn", Level::kWarn},
        {"warning", Level::kWarn},
        {"info", Level::kInfo},
        {"debug", Level::kDebug},
        {"trace", Level::kTrace},
        {"all", Level::kTrace},
        {"max", Level::kTrace},
        /* clang-format on */
    };

    for (const auto& entry : kLevelNames) {
        if (Ascii::EqualsIgnoreCase(entry.name, str)) {
            return entry.level;
        }
    }
    return std::nullopt;
}

//==============================================================================
//
// Logging Internals
//
//==============================================================================

void Logging::Internal::FatalExit()
{
    ::exit(EXIT_FAILURE);
}

void Logging::Internal::AssertFailure(const SourceLocation* location, const char* expr)
{
    Logging::Log(Level::kFatal, location, "Assertion failed: {}", expr);
    ::abort();
}

void Logging::Internal::DoLogV(Level level, const SourceLocation* location, eastl::string_view fmt,
                               Formatting::FormatArgs args)
{
    struct ::timespec time;
    const struct ::timespec* timePtr = nullptr;

    if (!s_loggerState.sourceLocationEnabled) {
        location = nullptr;
    }
    if (s_loggerState.timestampsEnabled && ::timespec_get(&time, TIME_UTC)) {
        timePtr = &time;
    }

    tls_currentlyLogging = true;
    Core::ScopeExit _deferUnlock = [] { tls_currentlyLogging = false; };
    Internal::PlatformLog(level, location, fmt, args, timePtr);
}

//==============================================================================
//
// Message Formatting
//
//==============================================================================

namespace {

    char ToHexDigit(u8 value)
    {
        static const char kHexDigits[] = "0123456789abcdef";
        return kHexDigits[value & 0xF];
    }

    // Append characters to the message buffer
    class MessageWriter final : public Formatting::Writer {
    public:
        explicit MessageWriter(Logging::MessageBuffer& buffer) : m_buffer(buffer) {}

    protected:
        void DoWrite(const char* chars, size_t count) override { m_buffer.append(chars, count); }

    private:
        Logging::MessageBuffer& m_buffer;
    };

    // Writer that sanitizes the message before appending to the buffer
    class SanitizedMessageWriter final : public Formatting::Writer {
    public:
        explicit SanitizedMessageWriter(Logging::MessageBuffer& buffer) : m_buffer(buffer) {}

    protected:
        void DoWrite(const char* chars, size_t count) override
        {
            const char* pos = chars;
            const char* end = chars + count;

            while (pos < end) {
                if (Ascii::IsControlChar(*pos)) {
                    // Escape control chars to prevent clobbering the terminal state.
                    switch (*pos) {
                        case '\t':
                            m_buffer.push_back('\t');
                            break;
                        case '\r':
                            if (end - pos >= 2 && pos[1] == '\n') {
                                pos++;
                            }
                            [[fallthrough]];
                        case '\n':
                            if (end - pos >= 2 && pos[1] != '\n') {
                                // Indent continuation lines.
                                m_buffer.append("\n  ", 3);
                            }
                            break;
                        default: {
                            char seq[3] = {'%', ToHexDigit(static_cast<u8>(*pos) >> 4),
                                           ToHexDigit(static_cast<u8>(*pos) & 0xF)};
                            m_buffer.append(seq, 3);
                            break;
                        }
                    }
                    pos++;
                } else {
                    // Append non-control characters directly to the buffer.
                    const char* runStart = pos++;
                    while (pos < end && !Ascii::IsControlChar(*pos)) {
                        pos++;
                    }
                    m_buffer.append(runStart, static_cast<size_t>(pos - runStart));
                }
            }
        }

    private:
        Logging::MessageBuffer& m_buffer;
    };

    void FormatTimestamp(MessageWriter& writer, Logging::FormatMessageFlags flags, const struct ::timespec* time)
    {
        char timeBuf[64];
        struct ::tm tm;
        u32 millis = static_cast<u32>(time->tv_nsec / 1000000);

        if (s_loggerState.localTimeEnabled) {
#ifdef _WIN32
            ::localtime_s(&tm, &time->tv_sec);
#else
            ::localtime_r(&time->tv_sec, &tm);
#endif
        } else {
#ifdef _WIN32
            ::gmtime_s(&tm, &time->tv_sec);
#else
            ::gmtime_r(&time->tv_sec, &tm);
#endif
        }

        if (::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm)) {
            if (All(flags, Logging::FormatMessageFlags::kAnsiColors)) {
                Formatting::FormatTo(writer, "\033[2;32m[{}.{:03}]\033[0m ", Formatting::ParseMode::kPermissive,
                                     timeBuf, millis);
            } else {
                Formatting::FormatTo(writer, "[{}.{:03}] ", Formatting::ParseMode::kPermissive, timeBuf, millis);
            }
        }
    }

    void FormatLogLevel(MessageWriter& writer, Logging::FormatMessageFlags flags, Logging::Level level)
    {
        eastl::string_view str;

        if (All(flags, Logging::FormatMessageFlags::kAnsiColors)) {
            switch (level) {
                case Logging::Level::kFatal:
                    str = "\033[1;31m[FATAL ERROR]\033[0m ";
                    break;
                case Logging::Level::kError:
                    str = "\033[1;31m[error]\033[0m ";
                    break;
                case Logging::Level::kWarn:
                    str = "\033[1;33m[warning]\033[0m ";
                    break;
                case Logging::Level::kInfo:
                    str = "\033[32m[info]\033[0m ";
                    break;
                case Logging::Level::kDebug:
                    str = "\033[36m[debug]\033[0m ";
                    break;
                case Logging::Level::kTrace:
                    str = "\033[34m[trace]\033[0m ";
                    break;
                default:
                    break;
            }
        } else {
            switch (level) {
                case Logging::Level::kFatal:
                    str = "[FATAL ERROR] ";
                    break;
                case Logging::Level::kError:
                    str = "[error] ";
                    break;
                case Logging::Level::kWarn:
                    str = "[warning] ";
                    break;
                case Logging::Level::kInfo:
                    str = "[info] ";
                    break;
                case Logging::Level::kDebug:
                    str = "[debug] ";
                    break;
                case Logging::Level::kTrace:
                    str = "[trace] ";
                    break;
                default:
                    break;
            }
        }

        if (str.empty()) {
            Formatting::FormatTo(writer, "[level {}] ", Formatting::ParseMode::kPermissive, static_cast<u32>(level));
        } else {
            writer.Write(str);
        }
    }

    void FormatSourceLocation(MessageWriter& writer, Logging::FormatMessageFlags flags,
                              const Logging::SourceLocation* location)
    {
        if (location->fileName) {
            if (All(flags, Logging::FormatMessageFlags::kAnsiColors)) {
                Formatting::FormatTo(writer, "\033[2;36m({}:{})\033[0m ", Formatting::ParseMode::kPermissive,
                                     location->fileName, location->lineNum);
            } else {
                Formatting::FormatTo(writer, "({}:{}) ", Formatting::ParseMode::kPermissive, location->fileName,
                                     location->lineNum);
            }
        }

        if (location->funcName) {
            if (All(flags, Logging::FormatMessageFlags::kAnsiColors)) {
                Formatting::FormatTo(writer, "\033[2;33m(in {})\033[0m ", Formatting::ParseMode::kPermissive,
                                     location->funcName);
            } else {
                Formatting::FormatTo(writer, "(in {}) ", Formatting::ParseMode::kPermissive, location->funcName);
            }
        }
    }

} // namespace

void Logging::FormatMessage(MessageBuffer& buffer, FormatMessageFlags flags, Level level,
                            const SourceLocation* location, eastl::string_view fmt, Formatting::FormatArgs args,
                            const struct ::timespec* time)
{
    MessageWriter writer{buffer};
    SanitizedMessageWriter sanitizedWriter{buffer};

    if (time) {
        ::FormatTimestamp(writer, flags, time);
    }

    ::FormatLogLevel(writer, flags, level);

    if (location) {
        ::FormatSourceLocation(writer, flags, location);
    }

    if (All(flags, Logging::FormatMessageFlags::kAnsiColors)) {
        switch (level) {
            case Level::kFatal:
                buffer.append("\033[31m");
                break;
            case Level::kTrace:
                buffer.append("\033[2m");
                break;
            default:
                break;
        }
    }

    Formatting::FormatToV(sanitizedWriter, fmt, Formatting::ParseMode::kPermissive, args);

    if (All(flags, Logging::FormatMessageFlags::kAnsiColors)) {
        buffer.append("\033[0m");
    }

    if (All(flags, Logging::FormatMessageFlags::kEndLine)) {
        buffer.push_back('\n');
    }
}
