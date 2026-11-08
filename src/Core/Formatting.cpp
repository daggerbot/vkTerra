// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include "Formatting.h"

//==============================================================================
//
// Misc Formatter Tests
//
//==============================================================================

static_assert(Formatting::FormatStringLiteral<"">() == "");
static_assert(Formatting::FormatStringLiteral<"asdf">() == "asdf");
static_assert(Formatting::FormatStringLiteral<"}}|{}{{{:}}}{}|{{", 'a', 'b', 'c'>() == "}|a{b}c|{");
static_assert(Formatting::FormatStringLiteral<"{2}{1:}{0}", 'a', 'b', 'c'>() == "cba");
static_assert(Formatting::FormatStringLiteral<"{}|{}", true, false>() == "true|false");

//==============================================================================
//
// Integer Formatter Tests
//
//==============================================================================

static_assert(Formatting::FormatStringLiteral<"{}", 0>() == "0");
static_assert(Formatting::FormatStringLiteral<"{}", 123>() == "123");
static_assert(Formatting::FormatStringLiteral<"{}", -123>() == "-123");

// Width and alignment
static_assert(Formatting::FormatStringLiteral<"{:5}", 123>() == "  123");
static_assert(Formatting::FormatStringLiteral<"{:<5}", 123>() == "123  ");
static_assert(Formatting::FormatStringLiteral<"{:<5}", -123>() == "-123 ");
static_assert(Formatting::FormatStringLiteral<"{:^5}", 123>() == " 123 ");
static_assert(Formatting::FormatStringLiteral<"{:^5}", -123>() == "-123 ");
static_assert(Formatting::FormatStringLiteral<"{:^6}", 123>() == "  123 ");
static_assert(Formatting::FormatStringLiteral<"{:^6}", -123>() == " -123 ");
static_assert(Formatting::FormatStringLiteral<"{:>5}", 123>() == "  123");
static_assert(Formatting::FormatStringLiteral<"{:>5}", -123>() == " -123");
static_assert(Formatting::FormatStringLiteral<"{:1}", 12345>() == "12345");

// Sign mode
static_assert(Formatting::FormatStringLiteral<"{:+}", 123>() == "+123");
static_assert(Formatting::FormatStringLiteral<"{:+}", -123>() == "-123");
static_assert(Formatting::FormatStringLiteral<"{:-}", 123>() == "123");
static_assert(Formatting::FormatStringLiteral<"{:-}", -123>() == "-123");
static_assert(Formatting::FormatStringLiteral<"{: }", 123>() == " 123");
static_assert(Formatting::FormatStringLiteral<"{: }", -123>() == "-123");
static_assert(Formatting::FormatStringLiteral<"{:_>}", 123>() == "123");
static_assert(Formatting::FormatStringLiteral<"{:_> }", 123>() == "_123");

// Fill char and separator
static_assert(Formatting::FormatStringLiteral<"{:_>5}", 123>() == "__123");
static_assert(Formatting::FormatStringLiteral<"{:05}", 123>() == "00123");
static_assert(Formatting::FormatStringLiteral<"{:04H}", 123>() == " 123"); // Should never start with a separator.
static_assert(Formatting::FormatStringLiteral<"{:05H}", 123>() == "0'123");
static_assert(Formatting::FormatStringLiteral<"{:05H:!}", 123>() == "0!123");

// Radix tests
static_assert(Formatting::FormatStringLiteral<"{:b}", 0b10101>() == "10101");
static_assert(Formatting::FormatStringLiteral<"{:#b}", 0b10101>() == "0b10101");
static_assert(Formatting::FormatStringLiteral<"{:Hb}", 0b10101>() == "1'0101");
static_assert(Formatting::FormatStringLiteral<"{:o}", 021337>() == "21337");
static_assert(Formatting::FormatStringLiteral<"{:#o}", 021337>() == "0o21337");
static_assert(Formatting::FormatStringLiteral<"{:Ho}", 021337>() == "2'1337");
static_assert(Formatting::FormatStringLiteral<"{:x}", 0x1BADD00D>() == "1badd00d");
static_assert(Formatting::FormatStringLiteral<"{:#x}", 0x1BADD00D>() == "0x1badd00d");
static_assert(Formatting::FormatStringLiteral<"{:Hx}", 0x1BADD00D>() == "1bad'd00d");
static_assert(Formatting::FormatStringLiteral<"{:#HX}", 0x1BADD00D>() == "0x1BAD'D00D");

// Extreme values
static_assert(Formatting::FormatStringLiteral<"{}", INT64_MAX>() == "9223372036854775807");
static_assert(Formatting::FormatStringLiteral<"{}", INT64_MIN>() == "-9223372036854775808");
static_assert(Formatting::FormatStringLiteral<"{}", UINT64_MAX>() == "18446744073709551615");

// Final boss
static_assert(Formatting::FormatStringLiteral<"{:?^+#16H:/X}", 0x1BADD00D>() == "??+0x1BAD/D00D??");
static_assert(Formatting::FormatStringLiteral<"{:?^+#016H:/X}", 0x1BADD00D>() == "+0x000/1BAD/D00D");
