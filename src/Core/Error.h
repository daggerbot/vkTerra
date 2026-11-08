// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <concepts>

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

#include "Formatting.h"

/* clang-format off */
#define CORE_FOR_EACH_STATUS(ENUM) \
    ENUM(kOk, "No error") \
    ENUM(kEndOfStream, "Unexpected end of stream") \
    ENUM(kInterrupted, "Operation interrupted") \
    ENUM(kInvalidArgument, "Invalid argument")
/* clang-format on */

namespace Core {

    class ErrorDomain;

    /// Underlying type for error codes.
    /// Must be large enough to hold all possible error code values.
    using ErrorCodeOrdinal = u32;

    /// General-purpose error code enumeration.
    enum class Status : u8 {
#define ENUM(IDENT, DESC) IDENT,
        CORE_FOR_EACH_STATUS(ENUM)
#undef ENUM
    };

    /// Formattable `errno` value wrapper.
    enum class Errno : int {};

#ifdef _WIN32
    /// Formattable Windows error code wrapper.
    enum class WinErrorCode : u32 {};
#endif

    /// Specializable traits for error code types.
    ///
    /// **Required members:**
    /// - `static constexpr char kDomainName[]`: Human-readable name of the error domain.
    ///
    /// **Optional members:**
    /// - `static bool IsEquivalent(T code, Core::Status status)`: Checks if the error code is semantiacally equivalent
    ///   to the given generic status. Defaults to never equivalent, except for `Core::Status::kOk` with codes
    ///   representing success, if omitted.
    /// - `static bool IsOk(T code)`: Checks if the error code represents success. Defaults to `true` for zero, `false`
    ///   for all other values, if omitted.
    template <typename T>
    struct ErrorCodeTraits;

    template <>
    struct ErrorCodeTraits<Status> {
        static constexpr char kDomainName[] = "generic";
        static constexpr bool IsEquivalent(Status code, Status status) { return code == status; }
    };

    template <>
    struct ErrorCodeTraits<Errno> {
        static constexpr char kDomainName[] = "errno";
        static bool IsEquivalent(Errno code, Status status);
    };

#ifdef _WIN32
    template <>
    struct ErrorCodeTraits<WinErrorCode> {
        static constexpr char kDomainName[] = "win32";
        static bool IsEquivalent(WinErrorCode code, Status status);
    };
#endif

    /// Concept for types with an associated \ref ErrorCodeTraits specialization.
    template <typename T>
    concept ErrorCodeEnum = std::is_enum_v<T> && sizeof(T) <= sizeof(ErrorCodeOrdinal) && requires {
        { ErrorCodeTraits<T>::kDomainName } -> std::convertible_to<const char*>;
    };

    namespace Internal {

        template <typename T>
        concept ErrorCodeTraits_IsEquivalent = requires(T code, Status status) {
            { ErrorCodeTraits<T>::IsEquivalent(code, status) } -> std::same_as<bool>;
        };

        template <typename T>
        concept ErrorCodeTraits_IsOk = requires(T code) {
            { ErrorCodeTraits<T>::IsOk(code) } -> std::same_as<bool>;
        };

    } // namespace Internal

    /// Represents an error domain corresponding to a specific error code type.
    class ErrorDomain {
    public:
        template <ErrorCodeEnum T>
        static constexpr const ErrorDomain& ForType()
        {
            return kDomain<T>;
        }

        constexpr bool operator==(const ErrorDomain& rhs) const { return this == &rhs; }

        constexpr void FormatDefault(ErrorCodeOrdinal code, Formatting::Writer& writer) const
        {
            m_pfnFormatDefault(code, writer);
        }

        constexpr const char* GetName() const { return m_domainName; }

        constexpr bool IsEquivalent(ErrorCodeOrdinal code, Status status) const
        {
            return m_pfnIsEquivalent(code, status);
        }

        constexpr bool IsOk(ErrorCodeOrdinal code) const { return m_pfnIsOk(code); }

    private:
        template <ErrorCodeEnum T>
        static const ErrorDomain kDomain;

        const char* m_domainName;
        void (*m_pfnFormatDefault)(ErrorCodeOrdinal code, Formatting::Writer& writer);
        bool (*m_pfnIsEquivalent)(ErrorCodeOrdinal code, Status status);
        bool (*m_pfnIsOk)(ErrorCodeOrdinal code);

        constexpr ErrorDomain() = default;
        constexpr ErrorDomain(const ErrorDomain& other) = default;
        constexpr ErrorDomain& operator=(const ErrorDomain& other) = default;

        template <ErrorCodeEnum T>
        static constexpr ErrorDomain MakeForType()
        {
            ErrorDomain domain{};
            domain.m_domainName = ErrorCodeTraits<T>::kDomainName;

            if constexpr (Formatting::Formattable<T>) {
                domain.m_pfnFormatDefault = [](ErrorCodeOrdinal code, Formatting::Writer& writer) {
                    Formatting::Formatter<T>{}.Format(static_cast<T>(code), writer);
                };
            } else {
                domain.m_pfnFormatDefault = [](ErrorCodeOrdinal code, Formatting::Writer& writer) {
                    Formatting::FormatToChecked<"Error code {} ({})">(
                        writer, static_cast<std::underlying_type_t<T>>(code), ErrorCodeTraits<T>::kDomainName);
                };
            }

            if constexpr (Internal::ErrorCodeTraits_IsEquivalent<T>) {
                if constexpr (Internal::ErrorCodeTraits_IsOk<T>) {
                    domain.m_pfnIsEquivalent = [](ErrorCodeOrdinal code, Status status) {
                        return status == Status::kOk && ErrorCodeTraits<T>::IsOk(static_cast<T>(code));
                    };
                } else {
                    domain.m_pfnIsEquivalent = [](ErrorCodeOrdinal code, Status status) {
                        return ErrorCodeTraits<T>::IsEquivalent(static_cast<T>(code), status);
                    };
                }
            } else {
                domain.m_pfnIsEquivalent = DefaultIsEquivalent;
            }

            if constexpr (Internal::ErrorCodeTraits_IsOk<T>) {
                domain.m_pfnIsOk = [](ErrorCodeOrdinal code) { return ErrorCodeTraits<T>::IsOk(static_cast<T>(code)); };
            } else {
                domain.m_pfnIsOk = DefaultIsOk;
            }

            return domain;
        }

        static constexpr bool DefaultIsEquivalent(ErrorCodeOrdinal code, Status status)
        {
            return code == 0 && status == Status::kOk;
        }

        static constexpr bool DefaultIsOk(ErrorCodeOrdinal code) { return code == 0; }
    };

    template <ErrorCodeEnum T>
    inline constexpr const ErrorDomain ErrorDomain::kDomain = ErrorDomain::MakeForType<T>();

    inline constexpr const ErrorDomain& kStatusDomain = ErrorDomain::ForType<Status>();
    inline constexpr const ErrorDomain& kErrnoDomain = ErrorDomain::ForType<Errno>();
#ifdef _WIN32
    inline constexpr const ErrorDomain& kWinErrorDomain = ErrorDomain::ForType<WinErrorCode>();
#endif

    /// Represents a numeric error code associated with a specific error domain.
    class ErrorCode {
    public:
        constexpr ErrorCode() = default;
        constexpr ErrorCode(const ErrorCode& other) = default;

        template <ErrorCodeEnum T>
        constexpr ErrorCode(T other)
            : m_value{static_cast<ErrorCodeOrdinal>(other)}, m_domain{&ErrorDomain::ForType<T>()}
        {
        }

        constexpr explicit ErrorCode(ErrorCodeOrdinal value, const ErrorDomain& domain)
            : m_value{value}, m_domain{&domain}
        {
        }

        constexpr ErrorCode& operator=(const ErrorCode& other) = default;
        constexpr explicit operator bool() const { return IsError(); }

        constexpr void Clear()
        {
            m_value = 0;
            m_domain = nullptr;
        }

        constexpr const ErrorDomain* GetDomain() const { return m_domain; }
        constexpr ErrorCodeOrdinal GetValue() const { return m_value; }

        constexpr bool IsEquivalent(Status status) const
        {
            if (m_domain) {
                return m_domain->IsEquivalent(m_value, status);
            } else {
                return status == Status::kOk;
            }
        }

        constexpr bool IsError() const { return !IsOk(); }
        constexpr bool IsOk() const { return !m_domain || m_domain->IsOk(m_value); }

    private:
        ErrorCodeOrdinal m_value = 0;
        const ErrorDomain* m_domain = nullptr;
    };

    /// General-purpose structured error type consisting of an error code, message, and inner error.
    /// Each field is optional but at least one should be set to represent an actual error.
    ///
    /// The `message` and `innerError` fields are heap allocated and owned. They should not be set in hot paths or
    /// expected error conditions such as socket failures. For such conditions, one should define an error code that
    /// suitably describes the condition without resotring to heap allocation.
    struct Error {
        eastl::string message{};
        ErrorCode code{};
        eastl::unique_ptr<Error> innerError{};

        explicit operator bool() const { return IsError(); }
        void Clear();
        bool HasEquivalent(Status status) const;
        bool IsError() const { return !IsOk(); }
        bool IsOk() const;
    };

} // namespace Core

namespace Formatting {

    template <>
    struct Formatter<Core::Status> : Formatter<eastl::string_view> {
        void Format(Core::Status status, Writer& writer) const;
    };

    template <>
    struct Formatter<Core::Errno> : Formatter<eastl::string_view> {
        void Format(Core::Errno errnoValue, Writer& writer) const;
    };

#ifdef _WIN32
    template <>
    struct Formatter<Core::WinErrorCode> : Formatter<eastl::string_view> {
        void Format(Core::WinErrorCode errorCode, Writer& writer) const;
    };
#endif

    template <>
    struct Formatter<Core::ErrorCode> {
        constexpr const char* Parse(eastl::string_view spec) { return spec.data(); }
        void Format(const Core::ErrorCode& code, Writer& writer) const;
    };

    template <>
    struct Formatter<Core::Error> {
        constexpr const char* Parse(eastl::string_view spec) { return spec.data(); }
        void Format(const Core::Error& error, Writer& writer) const;
    };

} // namespace Formatting
