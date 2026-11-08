// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <volk.h>

#include <Core/Types.h>

namespace GPU::Vulkan {

    struct VulkanRenderState {
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
    };

    extern VulkanRenderState g_vkState;

} // namespace GPU::Vulkan
