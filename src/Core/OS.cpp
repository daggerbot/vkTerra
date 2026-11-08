// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <stdlib.h>

#include "OS.h"

OSChar* OS::getenv(const OSChar* name)
{
#ifdef _WIN32
    return ::_wgetenv(name);
#else
    return ::getenv(name);
#endif
}
