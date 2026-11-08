// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <volk.h>

#include <Core/Error.h>
#include <Core/Formatting/Base.h>
#include <Core/Logging.h>

/// \def EXPECT_VK
/// Checks the result of a Vulkan function call and triggers a fatal error if it indicates failure.
#define EXPECT_VK(EXPR) \
    do { \
        ::VkResult _result_ = (EXPR); \
        if (_result_ != ::VK_SUCCESS) { \
            static const ::Logging::SourceLocation _location_ = {__FILE__, __func__, __LINE__}; \
            ::GPU::Vulkan::ExpectFailure(&_location_, #EXPR, _result_); \
        } \
    } while (0)

namespace Formatting {

    template <>
    struct Formatter<VkResult> : Formatter<eastl::string_view> {
        void Format(VkResult result, Writer& writer) const;
    };

} // namespace Formatting

namespace Core {

    template <>
    struct ErrorCodeTraits<VkResult> {
        static constexpr char kDomainName[] = "vulkan";
    };

    inline constexpr const ErrorDomain& kVulkanErrorDomain = ErrorDomain::ForType<VkResult>();

} // namespace Core

namespace GPU::Vulkan {

    [[noreturn]] void ExpectFailure(const Logging::SourceLocation* location, const char* expr, VkResult result);

} // namespace GPU::Vulkan
