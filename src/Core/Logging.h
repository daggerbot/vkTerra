// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <optional>
#include <type_traits>

#include "Formatting.h"
#include "Strings.h"

/// \def ASSERT
/// On debug builds (`NDEBUG` not defined), logs a fatal error and aborts the process if the expression is false.
/// On release builds (`NDEBUG` defined), does nothing.
#ifdef NDEBUG
# define ASSERT(EXPR) ((void)sizeof(EXPR))
#else
# define ASSERT(EXPR) \
     do { \
         if (!(EXPR)) { \
             static const ::Logging::SourceLocation _location_ = {__FILE__, __func__, __LINE__}; \
             ::Logging::Internal::AssertFailure(&_location_, #EXPR); \
         } \
     } while (0)
#endif

/// \def FATAL
/// Logs a fatal error message and terminates the program with an error code.
#ifdef MINSIZEREL
# define FATAL(...) ::Logging::LogFatal(nullptr, __VA_ARGS__)
#else
# define FATAL(...) \
     do { \
         static const ::Logging::SourceLocation _location_ = {__FILE__, __func__, __LINE__}; \
         ::Logging::LogFatal(&_location_, __VA_ARGS__); \
     } while (0)
#endif

/// \def LOG_ERROR
/// Logs an error message.
#ifdef MINSIZEREL
# define LOG_ERROR(...) ::Logging::Log(::Logging::Level::kError, nullptr, __VA_ARGS__)
#else
# define LOG_ERROR(...) \
     do { \
         static const ::Logging::SourceLocation _location_ = {__FILE__, __func__, __LINE__}; \
         ::Logging::Log(::Logging::Level::kError, &_location_, __VA_ARGS__); \
     } while (0)
#endif

/// \def LOG_WARN
/// Logs a warning message.
#ifdef MINSIZEREL
# define LOG_WARN(...) ::Logging::Log(::Logging::Level::kWarn, nullptr, __VA_ARGS__)
#else
# define LOG_WARN(...) \
     do { \
         static const ::Logging::SourceLocation _location_ = {__FILE__, __func__, __LINE__}; \
         ::Logging::Log(::Logging::Level::kWarn, &_location_, __VA_ARGS__); \
     } while (0)
#endif

/// \def LOG_INFO
/// Logs a neutral status message.
#ifdef MINSIZEREL
# define LOG_INFO(...) ::Logging::Log(::Logging::Level::kInfo, nullptr, __VA_ARGS__)
#else
# define LOG_INFO(...) \
     do { \
         static const ::Logging::SourceLocation _location_ = {__FILE__, __func__, __LINE__}; \
         ::Logging::Log(::Logging::Level::kInfo, &_location_, __VA_ARGS__); \
     } while (0)
#endif

/// \def LOG_DEBUG
/// Logs a message that may be useful for debugging purposes.
#ifdef MINSIZEREL
# define LOG_DEBUG(...) ((void)sizeof(::Logging::LogDiscard(__VA_ARGS__)))
#else
# define LOG_DEBUG(...) \
     do { \
         static const ::Logging::SourceLocation _location_ = {__FILE__, __func__, __LINE__}; \
         ::Logging::Log(::Logging::Level::kDebug, &_location_, __VA_ARGS__); \
     } while (0)
#endif

/// \def LOG_TRACE
/// Logs a trace message on debug builds only (`NDEBUG` not defined).
#if defined(NDEBUG) || defined(MINSIZEREL)
# define LOG_TRACE(...) ((void)sizeof(::Logging::LogDiscard(__VA_ARGS__)))
#else
# define LOG_TRACE(...) \
     do { \
         static const ::Logging::SourceLocation _location_ = {__FILE__, __func__, __LINE__}; \
         ::Logging::Log(::Logging::Level::kTrace, &_location_, __VA_ARGS__); \
     } while (0)
#endif

struct timespec;

/// Message logging system.
namespace Logging {

    enum class Level : u8 {
        kNone,
        kFatal,
        kError,
        kWarn,
        kInfo,
        kDebug,
        kTrace,
    };

    struct SourceLocation {
        const char* fileName;
        const char* funcName;
        u32 lineNum;
    };

    void InitLogger();
    void ShutDownLogger();
    void ShowConsoleWindow();
    void SetLogLevel(Level level);
    bool WouldLog(Level level);
    std::optional<Level> ParseLogLevel(OSStringView str);

    namespace Internal {

        void InitPlatformLogger();
        void ShutDownPlatformLogger();
        bool PlatformWouldLog(Level level);
        void PlatformLog(Level level, const SourceLocation* location, eastl::string_view fmt,
                         Formatting::FormatArgs args, const struct ::timespec* time);

        [[noreturn]] void FatalExit();
        [[noreturn]] void AssertFailure(const SourceLocation* location, const char* expr);
        void DoLogV(Level level, const SourceLocation* location, eastl::string_view fmt, Formatting::FormatArgs args);

    } // namespace Internal

    /// Logs a message at the specified log level.
    template <Formatting::Formattable... Args>
    void Log(Level level, const SourceLocation* location, eastl::string_view fmt, Args&&... args)
    {
        if (Logging::WouldLog(level)) {
            Internal::DoLogV(level, location, fmt, Formatting::MakeFormatArgs(std::forward<Args>(args)...));
        }
    }

    /// Logs a fatal message and terminates the program with an error code.
    template <Formatting::Formattable... Args>
    [[noreturn]] void LogFatal(const SourceLocation* location, eastl::string_view fmt, Args&&... args)
    {
        if (Logging::WouldLog(Level::kFatal)) {
            Internal::DoLogV(Level::kFatal, location, fmt, Formatting::MakeFormatArgs(std::forward<Args>(args)...));
        }
        Internal::FatalExit();
    }

    /// Has no definition. Used by logging macros that are disabled at compile time.
    template <Formatting::Formattable... Args>
    int LogDiscard(eastl::string_view fmt, Args&&... args);

} // namespace Logging
