// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <stdlib.h>
#include <string.h>

#include "Math.h"
#include "OS.h"

namespace {

    struct ThreadName {
        char buffer[32]{};
        u8 length = 0;
    };

    thread_local ThreadName tls_threadName{};

} // namespace

OSChar* OS::getenv(const OSChar* name)
{
#ifdef _WIN32
    return ::_wgetenv(name);
#else
    return ::getenv(name);
#endif
}

eastl::string_view OS::GetCurrentThreadDebugName()
{
    if (!tls_threadName.length) {
        return "unknown";
    }
    return {tls_threadName.buffer, tls_threadName.length};
}

void OS::SetCurrentThreadDebugName(eastl::string_view name)
{
    size_t length = Math::Min(name.size(), sizeof(tls_threadName.buffer) - 1);
    memcpy(tls_threadName.buffer, name.data(), length);
    tls_threadName.buffer[length] = 0;
    tls_threadName.length = static_cast<u8>(length);
}
