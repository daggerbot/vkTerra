// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <concepts>
#include <optional>
#include <span>

#include <EASTL/string.h>
#include <EASTL/string_view.h>

#include "Concepts.h"
#include "Formatting/Base.h"
#include "Formatting/IntFormatters.h"
#include "Formatting/MiscFormatters.h"
#include "Formatting/StringFormatters.h"
#include "Memory.h"

namespace Formatting {

    /// Specifies how invalid sequences should be handled when parsing a format string.
    enum class ParseMode {
        /// Any invalid sequences will cause a parsing error.
        /// \see \ref ParseResult::kFailure
        kStrict,
        /// Invalid sequences are replaced with verbatim text and parsing continues.
        /// \see \ref ParseResult::kFinishedWithReplacements
        kPermissive,
    };

    /// Represents the result of parsing a format string.
    enum class ParseResult {
        /// Parsing completed successfully without any errors.
        kSuccess,
        /// Parsing completed with replacements for invalid sequences.
        /// Only relevant when using \ref ParseMode::kPermissive.
        kFinishedWithReplacements,
        /// Parsing failed due to invalid sequences.
        /// Only relevant when using \ref ParseMode::kStrict.
        kFailure,
    };

    /// Format writer that discards all output.
    class NullWriter final : public Writer {
    protected:
        constexpr void DoWrite(const char*, size_t) override final {}
    };

    /// Format writer that counts the number of characters written.
    class CountingWriter final : public Writer {
    public:
        constexpr size_t GetCount() const { return m_count; }

    protected:
        constexpr void DoWrite(const char*, size_t count) override final { m_count += count; }

    private:
        size_t m_count = 0;
    };

    /// Format writer that writes to a fixed-size buffer.
    class FixedBufferWriter final : public Writer {
    public:
        constexpr FixedBufferWriter() = default;
        constexpr FixedBufferWriter(const FixedBufferWriter& other) = default;

        constexpr explicit FixedBufferWriter(char* buffer, size_t bufferSize)
            : m_buffer{buffer}, m_bufferSize{bufferSize}
        {
        }

        constexpr FixedBufferWriter& operator=(const FixedBufferWriter& other) = default;

        constexpr size_t GetCount() const { return m_count; }
        constexpr bool IsBufferExceeded() const { return m_count > m_bufferSize; }

    protected:
        constexpr void DoWrite(const char* chars, size_t count) override final
        {
            if (count && m_count < m_bufferSize) {
                size_t remaining = m_bufferSize - m_count;
                size_t toCopy = count < remaining ? count : remaining;

                // Help the compiler recognize this as a memcpy.
                char* RESTRICT dst = m_buffer + m_count;
                const char* RESTRICT src = chars;
                for (size_t i = 0; i < toCopy; i++) {
                    dst[i] = src[i];
                }
            }

            m_count += count;
        }

    private:
        char* m_buffer = nullptr;
        size_t m_bufferSize = 0;
        size_t m_count = 0;
    };

    /// Format writer that writes to an `eastl::string`.
    class StringWriter final : public Writer {
    public:
        constexpr explicit StringWriter(eastl::string& str) : m_string{str} {}

    protected:
        void DoWrite(const char* chars, size_t count) override final;

    private:
        eastl::string& m_string;
    };

    namespace Internal {

        /// Concept that checks if a type has a corresponding formatter.
        template <typename T>
        concept HasFormatter =
            requires { Formatter<T>{}; } && requires(Formatter<T> formatter, eastl::string_view spec) {
                { formatter.Parse(spec) } -> std::same_as<const char*>;
            } && requires(const Formatter<T> formatter, const T& value, Writer& writer) {
                formatter.Format(value, writer);
            };

        /// Interface for handling format argument sequences when formatting strings.
        class ArgFormatter {
        public:
            constexpr virtual ~ArgFormatter() = default;

            /// Formats the argument at the specified index and writes the result to the provided writer.
            /// \param index The index of the argument to format.
            /// \param spec The format string starting at the `spec` portion of a `{[index][:spec]}` sequence, or the
            ///             closing brace if no spec is provided.
            /// \param writer The writer to which the formatted argument should be written.
            /// \return A pointer to the first unparsed character in the format string after processing the argument.
            ///         If this does not point to a closing brace inside the `spec` string, it indicates a parsing
            ///         error.
            constexpr virtual const char* FormatArg(u32 index, eastl::string_view spec, Writer& writer) const = 0;
        };

        /// Parses the given format string and writes the formatted output to the provided writer and argument
        /// formatter.
        NO_INLINE constexpr ParseResult ParseFormatString(eastl::string_view fmt, ParseMode mode, Writer& writer,
                                                          const ArgFormatter& argFormatter)
        {
            const char* pos = fmt.data();
            const char* end = pos + fmt.size();
            u32 nextImplicitIndex = 0;
            bool hasImplicitIndex = false;
            bool hasExplicitIndex = false;
            bool hasReplacements = false;

            while (pos < end) {
                if (*pos == '{') {
                    // Check for a '{{' escape sequence.
                    const char* seqStart = pos++;
                    if (pos < end && *pos == '{') {
                        writer.Write('{');
                        pos++;
                        continue;
                    }

                    // Extract the argument index if present.
                    u32 index = 0;
                    bool seqHasErrors = false;
                    if (pos < end && *pos >= '0' && *pos <= '9') {
                        hasExplicitIndex = true;
                        if (hasImplicitIndex && mode == ParseMode::kStrict) {
                            // Strict mode doesn't allow mixing implicit and explicit argument indices.
                            return ParseResult::kFailure;
                        }
                        do {
                            u8 digit = static_cast<u8>(*pos - '0');
                            if (index > (UINT32_MAX - digit) / 10) {
                                if (mode == ParseMode::kStrict) {
                                    return ParseResult::kFailure;
                                } else {
                                    seqHasErrors = true;
                                }
                            }
                            index = index * 10 + digit;
                            pos++;
                        } while (pos < end && *pos >= '0' && *pos <= '9');
                    } else {
                        if (hasExplicitIndex) {
                            if (mode == ParseMode::kStrict) {
                                // Strict mode doesn't allow mixing implicit and explicit argument indices.
                                return ParseResult::kFailure;
                            } else {
                                seqHasErrors = true;
                            }
                        } else {
                            index = nextImplicitIndex++;
                            hasImplicitIndex = true;
                        }
                    }

                    // Call the formatter if everything is valid so far.
                    if (!seqHasErrors && pos < end && (*pos == ':' || *pos == '}')) {
                        if (*pos == ':') {
                            pos++;
                        }
                        const char* seqEnd = argFormatter.FormatArg(
                            index, eastl::string_view{pos, static_cast<size_t>(end - pos)}, writer);
                        if (seqEnd >= pos && seqEnd < end && *seqEnd == '}') {
                            // Successfully formatted the argument.
                            pos = seqEnd + 1;
                            continue;
                        } else if (mode == ParseMode::kStrict) {
                            return ParseResult::kFailure;
                        }
                    }

                    // If we reach here, it means there was an error in the sequence.
                    // Emit the invalid sequence verbatim.
                    while (pos < end && *pos != '}') {
                        pos++;
                    }
                    if (pos < end && *pos == '}') {
                        pos++;
                    }
                    writer.Write(seqStart, static_cast<size_t>(pos - seqStart));
                    hasReplacements = true;
                } else if (*pos == '}') {
                    // Check for a valid '}}' escape sequence.
                    pos++;
                    if (pos < end && *pos == '}') {
                        writer.Write('}');
                        pos++;
                    } else if (mode == ParseMode::kStrict) {
                        return ParseResult::kFailure;
                    } else {
                        writer.Write('}');
                        hasReplacements = true;
                    }
                } else {
                    // Emit any other character verbatim.
                    const char* runStart = pos++;
                    while (pos < end && *pos != '{' && *pos != '}') {
                        pos++;
                    }
                    writer.Write(runStart, static_cast<size_t>(pos - runStart));
                }
            }

            return hasReplacements ? ParseResult::kFinishedWithReplacements : ParseResult::kSuccess;
        }

        /// Helper struct to map a size to the corresponding floating-point type.
        /// \see \ref floatptr_t
        template <size_t N>
        struct SizedFloat;

        template <>
        struct SizedFloat<sizeof(float)> {
            using Type = float;
        };

        template <>
        struct SizedFloat<sizeof(double)> {
            using Type = double;
        };

        /// Floating-point type corresponding to the size of a pointer.
        using floatptr_t = SizedFloat<sizeof(void*)>::Type;

        /// Represents a format argument packed into a machine word by value or by reference.
        union PackedArg {
            bool b;
            char c;
            intptr_t i;
            uintptr_t u;
            floatptr_t f;
            void* p;
            const void* cp;
        };

        /// Identifies the major type of a format argument.
        enum class ArgTypeId : u8 {
            kBool,
            kChar,
            kInt,
            kUInt,
            kFloat,
            kPointer,
            kCharPtr,
            kCharArray,
            kStringView,
            kString,
            kCustom,
        };

        /// Holds additional data associated with a format argument type.
        union ArgTypeData {
            const char* (*customFormatFunc)(PackedArg arg, eastl::string_view spec, Writer& writer);
            size_t extent;
        };

        /// Holds information about a format argument type, including its major type and any associated data.
        struct ArgTypeInfo {
            ArgTypeId typeId;
            ArgTypeData data;
        };

        /// Formats a value using its corresponding formatter.
        template <typename T>
        constexpr const char* DoFormatArg(T value, eastl::string_view spec, Writer& writer)
        {
            Formatter<std::remove_cvref_t<T>> formatter{};
            const char* specEnd = formatter.Parse(spec);
            if (specEnd >= spec.data() && specEnd < spec.data() + spec.size() && *specEnd == '}') {
                formatter.Format(value, writer);
            }
            return specEnd;
        }

        /// Packs a format argument into a machine word by value or by reference.
        template <typename T>
        ALWAYS_INLINE constexpr PackedArg PackArg(T&& arg)
        {
            using Value = std::remove_cvref_t<T>;

            if constexpr (std::is_same_v<Value, bool>) {
                return PackedArg{.b = arg};
            } else if constexpr (std::is_same_v<Value, char>) {
                return PackedArg{.c = arg};
            } else if constexpr (Core::StrictIntegral<Value> && sizeof(Value) <= sizeof(void*)) {
                if constexpr (std::is_signed_v<Value>) {
                    return PackedArg{.i = arg};
                } else {
                    return PackedArg{.u = arg};
                }
            } else if constexpr (std::is_floating_point_v<Value> && sizeof(Value) <= sizeof(void*)) {
                return PackedArg{.f = arg};
            } else if constexpr (std::is_pointer_v<Value>
                                 && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<Value>>, void>)
            {
                return PackedArg{.cp = arg};
            } else if constexpr (std::is_pointer_v<Value>
                                 && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<Value>>, char>)
            {
                return PackedArg{.cp = arg};
            } else if constexpr (std::is_array_v<Value>
                                 && std::is_same_v<std::remove_cv_t<std::remove_extent_t<Value>>, char>)
            {
                return PackedArg{.cp = arg};
            } else {
                return PackedArg{.cp = &arg};
            }
        }

        /// Returns the type information for a given format argument type.
        /// \return A \ref ArgTypeInfo containing the type information for the format argument if it is formattable,
        ///         otherwise `void`.
        template <typename T>
        constexpr auto GetArgTypeInfo()
        {
            using Value = std::remove_cvref_t<T>;

            if constexpr (std::is_same_v<Value, bool>) {
                return ArgTypeInfo{ArgTypeId::kBool, {}};
            } else if constexpr (std::is_same_v<Value, char>) {
                return ArgTypeInfo{ArgTypeId::kChar, {}};
            } else if constexpr (Core::StrictIntegral<Value> && sizeof(Value) <= sizeof(void*)) {
                if constexpr (std::is_signed_v<Value>) {
                    return ArgTypeInfo{ArgTypeId::kInt, {}};
                } else {
                    return ArgTypeInfo{ArgTypeId::kUInt, {}};
                }
            } else if constexpr (std::is_floating_point_v<Value> && sizeof(Value) <= sizeof(void*)) {
                return ArgTypeInfo{ArgTypeId::kFloat, {}};
            } else if constexpr (std::is_pointer_v<Value>
                                 && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<Value>>, void>)
            {
                return ArgTypeInfo{ArgTypeId::kPointer, {}};
            } else if constexpr (std::is_pointer_v<Value>
                                 && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<Value>>, char>)
            {
                return ArgTypeInfo{ArgTypeId::kCharPtr, {}};
            } else if constexpr (std::is_array_v<Value>
                                 && std::is_same_v<std::remove_cv_t<std::remove_extent_t<Value>>, char>)
            {
                return ArgTypeInfo{ArgTypeId::kCharArray, {.extent = std::extent_v<Value>}};
            } else if constexpr (std::is_same_v<Value, eastl::string_view>) {
                return ArgTypeInfo{ArgTypeId::kStringView, {}};
            } else if constexpr (std::is_same_v<Value, eastl::string>) {
                return ArgTypeInfo{ArgTypeId::kString, {}};
            } else if (HasFormatter<Value>) {
                return ArgTypeInfo{ArgTypeId::kCustom,
                                   {.customFormatFunc = [](PackedArg arg, eastl::string_view spec, Writer& writer) {
                                       return Internal::DoFormatArg<const Value&>(
                                           *reinterpret_cast<const Value*>(arg.cp), spec, writer);
                                   }}};
            }
        }

        /// Formats a packed argument based on its type information.
        constexpr const char* FormatPackedArg(PackedArg arg, const ArgTypeInfo& typeInfo, eastl::string_view spec,
                                              Writer& writer)
        {
            switch (typeInfo.typeId) {
                case ArgTypeId::kBool:
                    return Internal::DoFormatArg<bool>(arg.b, spec, writer);
                case ArgTypeId::kChar:
                    return Internal::DoFormatArg<char>(arg.c, spec, writer);
                case ArgTypeId::kInt:
                    return Internal::DoFormatArg<intptr_t>(arg.i, spec, writer);
                case ArgTypeId::kUInt:
                    return Internal::DoFormatArg<uintptr_t>(arg.u, spec, writer);

                    // TODO: kFloat

                case ArgTypeId::kPointer:
                    return Internal::DoFormatArg<const void*>(arg.cp, spec, writer);
                case ArgTypeId::kCharPtr:
                    return Internal::DoFormatArg<const char*>(static_cast<const char*>(arg.cp), spec, writer);
                case ArgTypeId::kCharArray: {
                    const char* ptr = static_cast<const char*>(arg.cp);
                    size_t length = Formatter<eastl::string_view>::GetLengthBounded(ptr, typeInfo.data.extent);
                    return Internal::DoFormatArg<eastl::string_view>(eastl::string_view{ptr, length}, spec, writer);
                }
                case ArgTypeId::kStringView:
                    return Internal::DoFormatArg<eastl::string_view>(*static_cast<const eastl::string_view*>(arg.cp),
                                                                     spec, writer);
                case ArgTypeId::kString:
                    return Internal::DoFormatArg<eastl::string_view>(*static_cast<const eastl::string*>(arg.cp), spec,
                                                                     writer);
                case ArgTypeId::kCustom:
                    return typeInfo.data.customFormatFunc(arg, spec, writer);
                default:
                    // Invalid or unimplemented type ID.
                    return nullptr;
            }
        }

        /// Argument formatter that validates the value sequence without emitting anything.
        template <typename... Args>
        class ValidatingArgFormatter;

        template <>
        class ValidatingArgFormatter<> : public ArgFormatter {
        public:
            constexpr const char* FormatArg(u32, eastl::string_view, Writer&) const override final { return nullptr; }
        };

        template <typename Head, typename... Tail>
        class ValidatingArgFormatter<Head, Tail...> : public ArgFormatter {
        public:
            constexpr const char* FormatArg(u32 index, eastl::string_view spec, Writer& writer) const override final
            {
                if (index == 0) {
                    return Formatter<Head>{}.Parse(spec);
                } else {
                    return m_tail.FormatArg(index - 1, spec, writer);
                }
            }

        private:
            [[no_unique_address]] ValidatingArgFormatter<Tail...> m_tail;
        };

        /// Provides a compile-time list of argument type information for the specified types.
        template <typename... Args>
        struct ArgTypeList {
            static constexpr ArgTypeInfo kList[] = {Internal::GetArgTypeInfo<Args>()...};
        };

        template <>
        struct ArgTypeList<> {
            static constexpr const ArgTypeInfo* kList = nullptr;
        };

        /// Storage for a fixed-size array of packed arguments.
        template <size_t N>
        struct FormatArgsDataStorage {
            PackedArg data[N];

            template <typename... Args>
            ALWAYS_INLINE constexpr explicit FormatArgsDataStorage(Args&&... args)
            requires(sizeof...(Args) == N)
                : data{Internal::PackArg(std::forward<Args>(args))...}
            {
            }
        };

        template <>
        struct FormatArgsDataStorage<0> {
            static constexpr const PackedArg* data = nullptr;
        };

        /// Provides storage and construction for a fixed-size array of packed arguments.
        template <typename... Args>
        struct FormatArgsData : FormatArgsDataStorage<sizeof...(Args)> {
            template <typename... CtorArgs>
            ALWAYS_INLINE constexpr explicit FormatArgsData(CtorArgs&&... args)
            requires(sizeof...(CtorArgs) == sizeof...(Args))
                : FormatArgsDataStorage<sizeof...(Args)>{std::forward<CtorArgs>(args)...}
            {
            }
        };

    } // namespace Internal

    /// Matches types that can be formatted as text.
    /// \see \ref Formatter
    template <typename T>
    concept Formattable = requires {
        { Internal::GetArgTypeInfo<std::remove_cvref_t<T>>() } -> std::same_as<Internal::ArgTypeInfo>;
    };

    /// Validates a format string against the provided argument types.
    template <Formattable... Args>
    constexpr bool ValidateFormatString(eastl::string_view fmt)
    {
        NullWriter writer{};
        Internal::ValidatingArgFormatter<std::remove_cvref_t<Args>...> argFormatter{};
        return Internal::ParseFormatString(fmt, ParseMode::kStrict, writer, argFormatter) == ParseResult::kSuccess;
    }

    /// Represents a borrowed list of type-erased format arguments.
    class FormatArgs : public Internal::ArgFormatter {
    public:
        constexpr FormatArgs() = default;
        constexpr FormatArgs(const FormatArgs& other) = default;

        /// Constructs a `FormatArgs` object from the type returned by \ref MakeFormatArgs.
        template <Formattable... Args>
        constexpr FormatArgs(const Internal::FormatArgsData<Args...>& data)
            : m_data{data.data}, m_types{Internal::ArgTypeList<Args...>::kList}, m_count{sizeof...(Args)}
        {
        }

        constexpr FormatArgs& operator=(const FormatArgs& other) = default;

        constexpr const char* FormatArg(u32 index, eastl::string_view spec, Writer& writer) const override final
        {
            if (index >= m_count) {
                return nullptr;
            } else {
                return Internal::FormatPackedArg(m_data[index], m_types[index], spec, writer);
            }
        }

    private:
        const Internal::PackedArg* m_data = nullptr;
        const Internal::ArgTypeInfo* m_types = nullptr;
        size_t m_count = 0;
    };

    /// Makes a type-erased list of format arguments.
    template <Formattable... Args>
    constexpr auto MakeFormatArgs(Args&&... args)
    {
        return Internal::FormatArgsData<std::remove_cvref_t<Args>...>{std::forward<Args>(args)...};
    }

    /// Represents a string literal of a fixed size.
    /// Can be used to represent a non-type template parameter.
    template <size_t N>
    struct StringLiteral {
        static constexpr size_t kSize = N;
        char data[N]{};

        constexpr StringLiteral() = default;
        constexpr StringLiteral(const StringLiteral& other) = default;

        constexpr StringLiteral(const char (&str)[N])
        {
            for (size_t i = 0; i < N; ++i) {
                data[i] = str[i];
            }
        }

        constexpr StringLiteral& operator=(const StringLiteral& other) = default;

        template <size_t M>
        constexpr bool operator==(const StringLiteral<M>& rhs) const
        {
            return *this == rhs.data;
        }

        template <size_t M>
        constexpr bool operator==(const char (&rhs)[M]) const
        {
            if constexpr (N != M) {
                return false;
            }
            for (size_t i = 0; i < N; i++) {
                if (data[i] != rhs[i]) {
                    return false;
                }
            }
            return true;
        }

        constexpr eastl::string_view GetFullView() const { return {data, N}; }
        constexpr eastl::string_view GetUnterminatedView() const { return {data, data[N - 1] ? N : N - 1}; }
    };

    /// Formats a string with type-erased arguments by writing the result to the provided \ref Writer.
    constexpr ParseResult FormatToV(Writer& writer, eastl::string_view fmt, ParseMode mode, FormatArgs args)
    {
        return Internal::ParseFormatString(fmt, mode, writer, args);
    }

    /// Formats a string with the provided arguments by writing the result to the provided \ref Writer.
    template <Formattable... Args>
    constexpr ParseResult FormatTo(Writer& writer, eastl::string_view fmt, ParseMode mode, Args&&... args)
    {
        return Formatting::FormatToV(writer, fmt, mode, Formatting::MakeFormatArgs(std::forward<Args>(args)...));
    }

    /// Formats a string with the provided arguments by writing the result to the provided \ref Writer.
    /// The format string is validated at compile-time.
    template <StringLiteral kFmt, Formattable... Args>
    constexpr void FormatToChecked(Writer& writer, Args&&... args)
    requires(Formatting::ValidateFormatString<Args...>(kFmt.GetUnterminatedView()))
    {
        Formatting::FormatTo(writer, kFmt.GetUnterminatedView(), ParseMode::kStrict, std::forward<Args>(args)...);
    }

    /// Computes the length of the formatted string with type-erased arguments if successful, or \ref std::nullopt if
    /// formatting fails.
    constexpr std::optional<size_t> GetFormattedLengthV(eastl::string_view fmt, ParseMode mode, FormatArgs args)
    {
        CountingWriter writer{};
        ParseResult result = Formatting::FormatToV(writer, fmt, mode, args);
        if (result == ParseResult::kFailure) {
            return std::nullopt;
        } else {
            return writer.GetCount();
        }
    }

    /// Computes the length of the formatted string with the provided arguments if successful, or \ref std::nullopt if
    /// formatting fails.
    template <Formattable... Args>
    constexpr std::optional<size_t> GetFormattedLength(eastl::string_view fmt, ParseMode mode, Args&&... args)
    {
        return Formatting::GetFormattedLengthV(fmt, mode, Formatting::MakeFormatArgs(std::forward<Args>(args)...));
    }

    /// Computes the length of the formatted string with the provided arguments.
    /// The format string is validated at compile-time.
    template <StringLiteral kFmt, Formattable... Args>
    constexpr size_t GetFormattedLengthChecked(Args&&... args)
    requires(Formatting::ValidateFormatString<Args...>(kFmt.GetUnterminatedView()))
    {
        return *Formatting::GetFormattedLength(kFmt.GetUnterminatedView(), ParseMode::kStrict,
                                               std::forward<Args>(args)...);
    }

    /// Formats a string literal at compile time.
    template <StringLiteral kFmt, auto... kArgs>
    consteval auto FormatStringLiteral()
    requires(Formatting::ValidateFormatString<decltype(kArgs)...>(kFmt.GetUnterminatedView()))
    {
        constexpr size_t kLength = Formatting::GetFormattedLengthChecked<kFmt>(kArgs...);
        StringLiteral<kLength + 1> outString{};
        FixedBufferWriter writer{outString.data, kLength};
        Formatting::FormatToChecked<kFmt>(writer, kArgs...);
        return outString;
    }

    /// Formats to a string with type-erased arguments.
    ParseResult FormatToStringV(eastl::string& outString, eastl::string_view fmt, ParseMode mode, FormatArgs args);

    /// Formats to a string with the provided arguments.
    template <Formattable... Args>
    ParseResult FormatToString(eastl::string& outString, eastl::string_view fmt, ParseMode mode, Args&&... args)
    {
        return Formatting::FormatToStringV(outString, fmt, mode,
                                           Formatting::MakeFormatArgs(std::forward<Args>(args)...));
    }

    /// Formats to a string with the provided arguments, validating the format string at compile time.
    template <StringLiteral kFmt, Formattable... Args>
    void FormatToStringChecked(eastl::string& outString, Args&&... args)
    requires(Formatting::ValidateFormatString<Args...>(kFmt.GetUnterminatedView()))
    {
        Formatting::FormatToString(outString, kFmt.GetUnterminatedView(), ParseMode::kStrict,
                                   std::forward<Args>(args)...);
    }

    /// Formats a string with the provided arguments, validating the format string at compile time.
    template <StringLiteral kFmt, Formattable... Args>
    eastl::string FormatStringChecked(Args&&... args)
    requires(Formatting::ValidateFormatString<Args...>(kFmt.GetUnterminatedView()))
    {
        eastl::string outString;
        Formatting::FormatToStringChecked<kFmt>(outString, std::forward<Args>(args)...);
        return outString;
    }

    /// Formats a scratch string with type-erased arguments.
    std::optional<eastl::string_view> FormatScratchStringV(eastl::string_view fmt, ParseMode mode, FormatArgs args);

    /// Formats a scratch string with the provided arguments.
    template <Formattable... Args>
    std::optional<eastl::string_view> FormatScratchString(eastl::string_view fmt, ParseMode mode, Args&&... args)
    {
        return Formatting::FormatScratchStringV(fmt, mode, Formatting::MakeFormatArgs(std::forward<Args>(args)...));
    }

    /// Formats a scratch string with the provided arguments, validating the format string at compile time.
    template <StringLiteral kFmt, Formattable... Args>
    eastl::string_view FormatScratchStringChecked(Args&&... args)
    requires(Formatting::ValidateFormatString<Args...>(kFmt.GetUnterminatedView()))
    {
        return Formatting::FormatScratchString(kFmt.GetUnterminatedView(), ParseMode::kStrict,
                                               std::forward<Args>(args)...);
    }

} // namespace Formatting
