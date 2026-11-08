// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <limits>
#include <span>
#include <type_traits>

#include <EASTL/string_view.h>

#include "../Concepts.h"
#include "Base.h"

namespace Formatting {

    /// Integer formatter implementation.
    struct IntFormatter {
        enum class Alignment : u8 { kLeft, kCenter, kRight };
        enum class DigitCase : u8 { kLower, kUpper };
        enum class SignMode : u8 { kNormal, kAlways, kFillPositive };

        static inline constexpr u8 kMinRadix = 2;
        static inline constexpr u8 kMaxRadix = 36;
        static inline constexpr u32 kMaxWidth = 1024;

        Alignment alignment = Alignment::kRight;
        DigitCase digitCase = DigitCase::kLower;
        u8 digitGrouping = 3;
        char fillChar = ' ';
        eastl::string_view prefix{};
        u8 radix = 10;
        char separator = 0;
        SignMode signMode = SignMode::kNormal;
        u32 width = 0;
        bool zeroFill = false;

        constexpr virtual void Reset() { *this = {}; }

        constexpr const char* Parse(eastl::string_view spec)
        {
            const char* pos = spec.data();
            const char* end = pos + spec.size();
            Reset();

            // fillChar
            if (end - pos >= 2) {
                switch (pos[1]) {
                    case '<':
                    case '^':
                    case '>':
                        fillChar = *pos++;
                        break;
                    default:
                        break;
                }
            }

            // alignment
            if (pos < end) {
                switch (*pos) {
                    case '<':
                        alignment = Alignment::kLeft;
                        pos++;
                        break;
                    case '^':
                        alignment = Alignment::kCenter;
                        pos++;
                        break;
                    case '>':
                        alignment = Alignment::kRight;
                        pos++;
                        break;
                    default:
                        break;
                }
            }

            // signMode
            if (pos < end) {
                switch (*pos) {
                    case '+':
                        signMode = SignMode::kAlways;
                        pos++;
                        break;
                    case '-':
                        signMode = SignMode::kNormal;
                        pos++;
                        break;
                    case ' ':
                        signMode = SignMode::kFillPositive;
                        pos++;
                        break;
                    default:
                        break;
                }
            }

            // usePrefix
            bool usePrefix = false;
            if (pos < end && *pos == '#') {
                usePrefix = true;
                pos++;
            }

            // zeroFill
            if (pos < end && *pos == '0') {
                zeroFill = true;
                pos++;
            }

            // width
            while (pos < end && *pos >= '0' && *pos <= '9') {
                u8 digit = static_cast<u8>(*pos - '0');
                if (width > (kMaxWidth - digit) / 10) {
                    return pos;
                }
                width = width * 10 + digit;
                pos++;
            }

            // {:H} to use the default separator (').
            // {:H:<separator>} to use a custom separator.
            if (pos < end && *pos == 'H') {
                separator = '\'';
                pos++;
                if (end - pos >= 2 && pos[0] == ':') {
                    separator = pos[1];
                    pos += 2;
                }
            }

            // radix and others
            if (pos < end) {
                switch (*pos) {
                    case 'B':
                    case 'b':
                        radix = 2;
                        digitGrouping = 4;
                        if (usePrefix) {
                            prefix = "0b";
                        }
                        pos++;
                        break;
                    case 'O':
                    case 'o':
                        radix = 8;
                        digitGrouping = 4;
                        if (usePrefix) {
                            prefix = "0o";
                        }
                        pos++;
                        break;
                    case 'D':
                    case 'I':
                    case 'd':
                    case 'i':
                        radix = 10;
                        digitGrouping = 3;
                        pos++;
                        break;
                    case 'X':
                    case 'x':
                        radix = 16;
                        digitCase = *pos == 'X' ? DigitCase::kUpper : DigitCase::kLower;
                        digitGrouping = 4;
                        if (usePrefix) {
                            prefix = "0x";
                        }
                        pos++;
                        break;
                    default:
                        break;
                }
            }

            return pos;
        }

        template <Core::IntegralNonBool T>
        constexpr void Format(T value, Writer& writer) const
        {
            static_assert(std::numeric_limits<T>::radix == 2);
            char digitBuffer[std::numeric_limits<T>::digits];
            std::span<char> digits = FillDigits(value, digitBuffer);
            DoFormat(digits, value < 0, writer);
        }

    private:
        constexpr char DigitToChar(u8 digit) const
        {
            if (digit < 10) {
                return static_cast<char>('0' + digit);
            } else if (digitCase == DigitCase::kUpper) {
                return static_cast<char>('A' - 10 + digit);
            } else {
                return static_cast<char>('a' - 10 + digit);
            }
        }

        template <Core::IntegralNonBool T>
        constexpr std::span<char> FillDigits(T value, std::span<char> buffer) const
        {
            char* end = buffer.data() + buffer.size();
            char* pos = end; // Fill from the back.

            if (value) {
                if constexpr (std::is_signed_v<T>) {
                    if (value < 0) {
                        *--pos = DigitToChar(static_cast<u8>(-(value % -static_cast<T>(radix))));
                        value /= -static_cast<T>(radix);
                    }
                }
                while (value) {
                    *--pos = DigitToChar(static_cast<u8>(value % static_cast<T>(radix)));
                    value /= static_cast<T>(radix);
                }
            } else {
                *--pos = '0';
            }

            return {pos, static_cast<size_t>(end - pos)};
        }

        NO_INLINE constexpr void DoFormat(std::span<const char> digits, bool negative, Writer& writer) const
        {
            // Determine the final sign char, if any.
            char signChar = 0;
            if (negative) {
                signChar = '-';
            } else {
                switch (signMode) {
                    case SignMode::kAlways:
                        signChar = '+';
                        break;
                    case SignMode::kFillPositive:
                        signChar = fillChar;
                        break;
                    default:
                        break;
                }
            }
            u8 signWidth = signChar ? 1 : 0;

            // Count digits and separators.
            size_t numDigits = digits.size();
            size_t numDigitsAndSeparators = numDigits;
            if (separator) {
                numDigitsAndSeparators += (numDigits - 1) / digitGrouping;
            }
            size_t significantDigitsAndSeparators = numDigitsAndSeparators;

            // Zero-fill if specified.
            if (zeroFill && width > signWidth + prefix.size() + numDigitsAndSeparators) {
                numDigitsAndSeparators = width - signWidth - prefix.size();
                if (separator && numDigitsAndSeparators % (digitGrouping + 1) == 0) {
                    // Never start with a separator.
                    numDigitsAndSeparators--;
                }
            }

            // Fill to specified width.
            size_t leftPad = 0;
            size_t rightPad = 0;
            if (width > signWidth + prefix.size() + numDigitsAndSeparators) {
                size_t pad = width - signWidth - prefix.size() - numDigitsAndSeparators;
                switch (alignment) {
                    case Alignment::kLeft:
                        rightPad = pad;
                        break;
                    case Alignment::kCenter:
                        rightPad = (pad + signWidth) >> 1;
                        leftPad = pad - rightPad;
                        break;
                    default:
                        leftPad = pad;
                        break;
                }
            }

            // Finally format the output.
            if (leftPad) {
                writer.RepeatChar(fillChar, leftPad);
            }
            if (signChar) {
                writer.Write(signChar);
            }
            if (!prefix.empty()) {
                writer.Write(prefix);
            }

            const char* pos = digits.data();
            for (size_t remaining = numDigitsAndSeparators; remaining; remaining--) {
                if (separator && remaining % (digitGrouping + 1) == 0) {
                    writer.Write(separator);
                } else if (remaining > significantDigitsAndSeparators) {
                    writer.Write('0');
                } else {
                    writer.Write(*pos++);
                }
            }

            if (rightPad) {
                writer.RepeatChar(fillChar, rightPad);
            }
        }
    };

    /// Integer formatter.
    template <Core::IntegralNonBool T>
    struct Formatter<T> : IntFormatter {
        constexpr void Format(T value, Writer& writer) const
        {
            if constexpr (sizeof(T) >= sizeof(intptr_t)) {
                IntFormatter::Format<T>(value, writer);
            } else if constexpr (std::is_signed_v<T>) {
                IntFormatter::Format<intptr_t>(value, writer);
            } else {
                IntFormatter::Format<uintptr_t>(value, writer);
            }
        }
    };

    /// Pointer formatter.
    template <>
    struct Formatter<void*> : Formatter<uintptr_t> {
        constexpr Formatter() { Reset(); }

        constexpr void Reset() override
        {
            Formatter<uintptr_t>::Reset();
            digitGrouping = 4;
            prefix = "0x";
            radix = 16;
        }

        constexpr void Format(const void* value, Writer& writer) const
        {
            Formatter<uintptr_t>::Format(reinterpret_cast<uintptr_t>(value), writer);
        }
    };

    /// Pointer formatter.
    template <>
    struct Formatter<const void*> : Formatter<void*> {};

} // namespace Formatting
