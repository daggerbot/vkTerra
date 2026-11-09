// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <volk.h>

#include <Core/Types.h>

namespace Gpu::Vulkan {

    struct VulkanRenderState {
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentationQueue = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;

        u32 graphicsQueueFamilyIndex = 0;
        u32 presentationQueueFamilyIndex = 0;
        VkFormat surfaceImageFormat = VK_FORMAT_UNDEFINED;
    };

    extern VulkanRenderState g_vkState;

} // namespace Gpu::Vulkan
