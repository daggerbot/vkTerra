// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "CompilerDefs.h"

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;

// Preferred character for OS interfaces.
#ifdef _WIN32
# define OSSTR_(s) L##s
# define OSSTR(s) OSSTR_(s)
using OSChar = wchar_t;
#else
# define OSSTR(s) s
using OSChar = char;
#endif
