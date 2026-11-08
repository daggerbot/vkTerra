// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <EASTL/string_view.h>

#include "../Types.h"

/// Text formatting utilities.
namespace Formatting {

    /// Abstract class for writing formatted text to an output destination.
    class Writer {
    public:
        constexpr virtual ~Writer() = default;

        constexpr virtual void Flush() {}

        constexpr void RepeatChar(char ch, size_t count)
        {
            while (count--) {
                DoWrite(&ch, 1);
            }
        }

        constexpr void Write(const char* str, size_t count) { DoWrite(str, count); }

        constexpr void Write(char ch) { DoWrite(&ch, 1); }

        constexpr void Write(const char* str)
        {
            if (str) {
                Write(eastl::string_view{str});
            }
        }

        constexpr void Write(eastl::string_view str) { DoWrite(str.data(), str.size()); }

    protected:
        constexpr virtual void DoWrite(const char* chars, size_t count) = 0;
    };

    /// Specializable type for formatting values of type `T` as text.
    ///
    /// Specializations must define at least the following members:
    /// - A default constructor, even if implicit.
    /// - `const char* parse(eastl::string_view spec)`: Parses the format specification string and returns a pointer to
    ///   the first unparsed character. If the returned pointer does not point to a closing brace inside the `spec`
    ///   string, the format specification is considered invalid. To support compile-time format checking, this function
    ///   should be `constexpr`.
    /// - `void format(const T& value, Writer& writer) const`: Formats the given value using the current settings and
    ///   writes it to the provided writer. To support compile-time string literal formatting, this function should be
    ///   `constexpr`.
    template <typename T>
    struct Formatter;

} // namespace Formatting
