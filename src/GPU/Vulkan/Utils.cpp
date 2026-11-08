// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <Core/Formatting.h>

#include "Utils.h"

//==============================================================================
//
// VkResult Utilities
//
//==============================================================================

void Formatting::Formatter<VkResult>::Format(VkResult result, Writer& writer) const
{
    const char* desc = nullptr;

    switch (result) {
#define CASE(NAME) \
    case NAME: \
        desc = #NAME; \
        break;

        // v1.0
        CASE(VK_SUCCESS)
        CASE(VK_TIMEOUT)
        CASE(VK_EVENT_SET)
        CASE(VK_EVENT_RESET)
        CASE(VK_INCOMPLETE)
        CASE(VK_ERROR_OUT_OF_HOST_MEMORY)
        CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY)
        CASE(VK_ERROR_INITIALIZATION_FAILED)
        CASE(VK_ERROR_DEVICE_LOST)
        CASE(VK_ERROR_MEMORY_MAP_FAILED)
        CASE(VK_ERROR_LAYER_NOT_PRESENT)
        CASE(VK_ERROR_EXTENSION_NOT_PRESENT)
        CASE(VK_ERROR_FEATURE_NOT_PRESENT)
        CASE(VK_ERROR_INCOMPATIBLE_DRIVER)
        CASE(VK_ERROR_TOO_MANY_OBJECTS)
        CASE(VK_ERROR_FORMAT_NOT_SUPPORTED)
        CASE(VK_ERROR_FRAGMENTED_POOL)
        CASE(VK_ERROR_UNKNOWN)
        CASE(VK_ERROR_VALIDATION_FAILED)

#undef CASE
        default:
            break;
    }

    if (desc) {
        writer.Write(desc);
    } else {
        Formatting::FormatToChecked<"Vulkan error code {}">(writer,
                                                            static_cast<std::underlying_type_t<VkResult>>(result));
    }
}

void GPU::Vulkan::ExpectFailure(const Logging::SourceLocation* location, const char* expr, VkResult result)
{
    Logging::LogFatal(location, "Vulkan call failed: {} => {}", expr, result);
}
