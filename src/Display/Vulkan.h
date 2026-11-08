// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <span>

#include <volk.h>

#include <Core/Types.h>

namespace Display::Vulkan {

    std::span<const char* const> GetRequiredInstanceExtensions(); // static|scratch
    VkSurfaceKHR CreateSurface(VkInstance instance, const VkAllocationCallbacks* allocator);
    void DestroySurface(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* allocator);

} // namespace Display::Vulkan
