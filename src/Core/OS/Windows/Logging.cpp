// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <stdlib.h>
#include <windows.h>

#include <Core/Logging.h>
#include <Core/LoggingUtils.h>
#include <Core/ScopeExit.h>
#include <Core/Unicode.h>

namespace {

    struct WindowsLoggerState {
        CRITICAL_SECTION criticalSection;
        HANDLE stderrHandle = nullptr;
        bool stderrIsConsole = false;
        bool stderrAnsiColorsEnabled = false;
    };

    WindowsLoggerState s_winLoggerState{};

    bool ShouldEnableAnsiColors()
    {
        wchar_t* noColor = _wgetenv(L"NO_COLOR"); // <https://no-color.org>
        wchar_t* forceColor = _wgetenv(L"FORCE_COLOR"); // <https://force-color.org>
        if (noColor && noColor[0]) {
            return false;
        } else if (forceColor && forceColor[0]) {
            return true;
        } else {
            return s_winLoggerState.stderrIsConsole;
        }
    }

    void InitStderr()
    {
        s_winLoggerState.stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
        if (s_winLoggerState.stderrHandle == INVALID_HANDLE_VALUE) {
            s_winLoggerState.stderrHandle = nullptr;
        }

        DWORD consoleMode;
        if (s_winLoggerState.stderrHandle && GetConsoleMode(s_winLoggerState.stderrHandle, &consoleMode)) {
            SetConsoleMode(s_winLoggerState.stderrHandle,
                           ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            s_winLoggerState.stderrIsConsole = true;
        }

        s_winLoggerState.stderrAnsiColorsEnabled = ShouldEnableAnsiColors();
    }

} // namespace

void Logging::ShowConsoleWindow()
{
    if (!s_winLoggerState.stderrHandle) {
        AllocConsole();
        InitStderr();
    }
}

void Logging::Internal::InitPlatformLogger()
{
    InitializeCriticalSection(&s_winLoggerState.criticalSection);
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        InitStderr();
    }
}

void Logging::Internal::ShutDownPlatformLogger()
{
    DeleteCriticalSection(&s_winLoggerState.criticalSection);
}

bool Logging::Internal::PlatformWouldLog(Level level)
{
    return s_winLoggerState.stderrHandle || level == Level::kFatal;
}

void Logging::Internal::PlatformLog(Level level, const SourceLocation* location, eastl::string_view fmt,
                                    Formatting::FormatArgs args, const struct ::timespec* time)
{
    MessageBuffer buffer;
    FormatMessageFlags flags = FormatMessageFlags::kNone;
    bool messageFormatted = false;
    bool messageBoxShown = false;

    if (s_winLoggerState.stderrHandle) {
        if (s_winLoggerState.stderrAnsiColorsEnabled) {
            flags |= FormatMessageFlags::kAnsiColors;
        }

        Logging::FormatMessage(buffer, flags, level, location, fmt, args, time);
        messageFormatted = true;

        if (s_winLoggerState.stderrIsConsole) {
            auto callback = [&](eastl::wstring_view wmsg) {
                // Write the wide message to the stderr console.
                const wchar_t* pos = wmsg.data();
                DWORD remaining = static_cast<DWORD>(wmsg.size());
                DWORD written;
                while (remaining) {
                    if (!WriteConsoleW(s_winLoggerState.stderrHandle, pos, remaining, &written, nullptr)) {
                        break;
                    } else if (!written) {
                        break;
                    }
                    pos += written;
                    remaining -= written;
                }
                WriteConsoleW(s_winLoggerState.stderrHandle, L"\n", 1, nullptr, nullptr);

                // If ANSI colors are disabled, we can use this same wide message for MessageBoxW.
                if (level == Level::kFatal) {
                    MessageBoxW(nullptr, wmsg.data(), L"Fatal Error", MB_OK | MB_ICONERROR | MB_TASKMODAL);
                    messageBoxShown = true;
                }
            };
            Unicode::CallWithUtf8ToWide(eastl::string_view{buffer.data(), buffer.size()}, callback);
        } else {
            // Write the UTF-8 message to the stderr file/pipe.
            const char* pos = buffer.data();
            DWORD remaining = static_cast<DWORD>(buffer.size());
            DWORD written;
            while (remaining) {
                if (!WriteFile(s_winLoggerState.stderrHandle, pos, remaining, &written, nullptr)) {
                    break;
                } else if (!written) {
                    break;
                }
                pos += written;
                remaining -= written;
            }
            WriteFile(s_winLoggerState.stderrHandle, "\n", 1, nullptr, nullptr);
        }
    }

    if (level == Level::kFatal && !messageBoxShown) {
        // Reformat the message if it's not formatted or has ANSI sequences.
        if (!messageFormatted || s_winLoggerState.stderrAnsiColorsEnabled) {
            Logging::FormatMessage(buffer, FormatMessageFlags::kNone, level, location, fmt, args, time);
        }

        // Show the message box for fatal errors.
        auto callback = [&](eastl::wstring_view wmsg) {
            MessageBoxW(nullptr, wmsg.data(), L"Fatal Error", MB_OK | MB_ICONERROR | MB_TASKMODAL);
        };
        Unicode::CallWithUtf8ToWide(eastl::string_view{buffer.data(), buffer.size()}, callback);
    }
}
