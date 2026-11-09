// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <volk.h>

#include <Core/Types.h>

namespace Gpu::Vulkan {

    inline constexpr const VkAllocationCallbacks* kDefaultAllocationCallbacks = nullptr;
    inline constexpr u32 kMinApiVersion = VK_API_VERSION_1_0;

} // namespace Gpu::Vulkan
