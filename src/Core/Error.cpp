// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <errno.h>
#include <string.h>

#include "Error.h"
#include "Formatting.h"
#include "Unicode.h"

//==============================================================================
//
// Status
//
//==============================================================================

void Formatting::Formatter<Core::Status>::Format(Core::Status status, Writer& writer) const
{
    const char* desc = nullptr;

    switch (status) {
#define ENUM(IDENT, DESC) \
    case Core::Status::IDENT: \
        desc = DESC; \
        break;
        CORE_FOR_EACH_STATUS(ENUM)
#undef ENUM
        default:
            break;
    }

    if (desc) {
        writer.Write(desc);
    } else {
        Formatting::FormatToChecked<"Unknown status {}">(writer,
                                                         static_cast<std::underlying_type_t<Core::Status>>(status));
    }
}

//==============================================================================
//
// Errno
//
//==============================================================================

bool Core::ErrorCodeTraits<Core::Errno>::IsEquivalent(Errno code, Status status)
{
    int errnum = static_cast<int>(code);
    switch (status) {
        case Status::kOk:
            return errnum == 0;
        case Status::kInterrupted:
            return errnum == EINTR;
        case Status::kInvalidArgument:
            return errnum == EINVAL;
        default:
            return false;
    }
}

void Formatting::Formatter<Core::Errno>::Format(Core::Errno code, Writer& writer) const
{
#ifdef __GLIBC__
    char buffer[256];
    char* desc = ::strerror_r(static_cast<int>(code), buffer, sizeof(buffer));
    if (desc) {
        writer.Write(desc);
    }
#elif defined(__unix__)
    char buffer[256];
    if (::strerror_r(static_cast<int>(code), buffer, sizeof(buffer)) == 0) {
        writer.Write(buffer);
    }
#elif defined(_WIN32)
    wchar_t buffer[256];
    if (::_wcserror_s(buffer, std::size(buffer), static_cast<int>(code)) == 0) {
        Unicode::CallWithWideToUtf8(eastl::wstring_view{buffer}, [&](eastl::string_view msg) { writer.Write(msg); });
    }
#endif

    Formatting::FormatToChecked<"errno {}">(writer, static_cast<int>(code));
}

//==============================================================================
//
// ErrorCode
//
//==============================================================================

void Formatting::Formatter<Core::ErrorCode>::Format(const Core::ErrorCode& code, Writer& writer) const
{
    if (code.IsOk()) {
        writer.Write("No error");
    } else {
        code.GetDomain()->FormatDefault(code.GetValue(), writer);
    }
}

//==============================================================================
//
// Error
//
//==============================================================================

void Core::Error::Clear()
{
    code.Clear();
    message.clear();
    innerError.reset();
}

bool Core::Error::HasEquivalent(Status status) const
{
    if (IsOk()) {
        return status == Status::kOk;
    } else if (code.IsEquivalent(status)) {
        return true;
    } else if (innerError) {
        return innerError->HasEquivalent(status);
    } else {
        return false;
    }
}

bool Core::Error::IsOk() const
{
    return code.IsOk() && message.empty() && (!innerError || innerError->IsOk());
}

void Formatting::Formatter<Core::Error>::Format(const Core::Error& error, Writer& writer) const
{
    bool anythingWritten = false;

    if (error.IsOk()) {
        writer.Write("No error");
        return;
    }

    if (!error.message.empty()) {
        writer.Write(error.message);
        anythingWritten = true;
    }

    if (error.code) {
        if (anythingWritten) {
            writer.Write(": ");
        }
        error.code.GetDomain()->FormatDefault(error.code.GetValue(), writer);
        anythingWritten = true;
    }

    if (error.innerError && error.innerError->IsError()) {
        if (anythingWritten) {
            writer.Write("\nCaused by: ");
        }
        Format(*error.innerError, writer);
    }
}
