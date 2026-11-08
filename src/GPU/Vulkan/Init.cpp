// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <string.h>

#include <span>

#include <volk.h>

#include <Core/Formatting.h>
#include <Core/Logging.h>
#include <Core/Memory.h>
#include <Core/OS.h>
#include <Core/Strings.h>
#include <Display/Vulkan.h>
#include <GameDefs.h>

#include "../GPU.h"
#include "Init.h"
#include "State.h"
#include "Utils.h"

using GPU::Vulkan::g_vkState;

namespace {

    constexpr const char* kValidationLayerName = "VK_LAYER_KHRONOS_validation";

    struct StartupInfo {
        Logging::Level logLevel = Logging::Level::kWarn;
        bool debugUtilsRequested = false;
        bool validationRequested = false;
    };

    StartupInfo s_startupInfo{
#ifndef NDEBUG
        .debugUtilsRequested = true,
        .validationRequested = true,
#endif
    };

} // namespace

//==============================================================================
//
// Misc Utilities
//
//==============================================================================

namespace {

    bool CheckExtension(std::span<const VkExtensionProperties> extensions, const char* extensionName)
    {
        for (const auto& ext : extensions) {
            if (strcmp(ext.extensionName, extensionName) == 0) {
                return true;
            }
        }
        return false;
    }

    bool CheckLayer(std::span<const VkLayerProperties> layers, const char* layerName)
    {
        for (const auto& layer : layers) {
            if (strcmp(layer.layerName, layerName) == 0) {
                return true;
            }
        }
        return false;
    }

} // namespace

//==============================================================================
//
// Custom Formatters
//
//==============================================================================

namespace {

    template <typename T>
    struct FormatWrapper {
        T value;
    };

} // namespace

namespace Formatting {

    // Extension name list formatter.
    template <>
    struct Formatter<::FormatWrapper<std::span<const char* const>>> {
        constexpr const char* Parse(eastl::string_view spec) { return spec.data(); }
        void Format(const ::FormatWrapper<std::span<const char* const>>& wrapper, Writer& writer) const
        {
            bool first = true;
            for (const char* str : wrapper.value) {
                if (!first) {
                    writer.Write(' ');
                }
                writer.Write(str);
                first = false;
            }
        }
    };

    // Extension properties list formatter.
    template <>
    struct Formatter<::FormatWrapper<std::span<const VkExtensionProperties>>> {
        constexpr const char* Parse(eastl::string_view spec) { return spec.data(); }
        void Format(const ::FormatWrapper<std::span<const VkExtensionProperties>>& wrapper, Writer& writer) const
        {
            bool first = true;
            for (const auto& ext : wrapper.value) {
                if (!first) {
                    writer.Write(' ');
                }
                writer.Write(ext.extensionName);
                first = false;
            }
        }
    };

    // Layer properties list formatter.
    template <>
    struct Formatter<::FormatWrapper<std::span<const VkLayerProperties>>> {
        constexpr const char* Parse(eastl::string_view spec) { return spec.data(); }
        void Format(const ::FormatWrapper<std::span<const VkLayerProperties>>& wrapper, Writer& writer) const
        {
            bool first = true;
            for (const auto& layer : wrapper.value) {
                if (!first) {
                    writer.Write(' ');
                }
                writer.Write(layer.layerName);
                first = false;
            }
        }
    };

} // namespace Formatting

//==============================================================================
//
// Debug Utilities
//
//==============================================================================

namespace {

    void CheckDebugEnvironment()
    {
        OSChar* logLevelStr = OS::getenv(OSSTR("VULKAN_LOG_LEVEL"));
        if (logLevelStr && logLevelStr[0]) {
            auto logLevel = Logging::ParseLogLevel(logLevelStr);
            if (logLevel.has_value()) {
                s_startupInfo.logLevel = *logLevel;
            } else {
                LOG_WARN("VULKAN_LOG_LEVEL: Invalid log level: {}", logLevelStr);
            }
        }

        OSChar* debugUtilsStr = OS::getenv(OSSTR("VULKAN_DEBUG_UTILS"));
        if (debugUtilsStr && debugUtilsStr[0]) {
            auto enabled = Strings::ParseBoolPermissive(debugUtilsStr);
            if (enabled.has_value()) {
                s_startupInfo.debugUtilsRequested = *enabled;
            } else {
                LOG_WARN("VULKAN_DEBUG_UTILS: Invalid boolean: {}", debugUtilsStr);
            }
        }

        OSChar* validationStr = OS::getenv(OSSTR("VULKAN_VALIDATION"));
        if (validationStr && validationStr[0]) {
            auto enabled = Strings::ParseBoolPermissive(validationStr);
            if (enabled.has_value()) {
                s_startupInfo.validationRequested = *enabled;
            } else {
                LOG_WARN("VULKAN_VALIDATION: Invalid boolean: {}", validationStr);
            }
        }
    }

    VkBool32 DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
    {
        char categoryBuf[64];
        Logging::Level level = Logging::Level::kTrace;

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            level = Logging::Level::kError;
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            level = Logging::Level::kWarn;
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            level = Logging::Level::kInfo;
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            level = Logging::Level::kTrace;
        } else {
            return VK_FALSE;
        }

        categoryBuf[0] = 0;
        if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            strcat(categoryBuf, "[validation] ");
        }
        if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
            strcat(categoryBuf, "[performance] ");
        }

        Logging::Log(level, nullptr, "[vulkan] {}{}", categoryBuf, pCallbackData->pMessage);
        return VK_FALSE;
    }

    VkDebugUtilsMessengerCreateInfoEXT MakeDebugUtilsMessengerCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugUtilsMessengerCallback;

        if (s_startupInfo.logLevel >= Logging::Level::kError) {
            createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        }
        if (s_startupInfo.logLevel >= Logging::Level::kWarn) {
            createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        }
        if (s_startupInfo.logLevel >= Logging::Level::kInfo) {
            createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        }
        if (s_startupInfo.logLevel >= Logging::Level::kTrace) {
            createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        }

        return createInfo;
    }

    void CreateDebugUtilsMessenger()
    {
        if (g_vkState.debugUtilsMessenger || !s_startupInfo.debugUtilsRequested) {
            return;
        }

        LOG_DEBUG("Creating debug utils messenger");

        auto createInfo = MakeDebugUtilsMessengerCreateInfo();
        VkResult result = vkCreateDebugUtilsMessengerEXT(
            g_vkState.instance, &createInfo, GPU::Vulkan::kDefaultAllocationCallbacks, &g_vkState.debugUtilsMessenger);

        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to create Vulkan debug utils messenger: {}", result);
        }
    }

    void DestroyDebugUtilsMessenger()
    {
        if (g_vkState.debugUtilsMessenger) {
            LOG_DEBUG("Destroying debug utils messenger");
            vkDestroyDebugUtilsMessengerEXT(g_vkState.instance, g_vkState.debugUtilsMessenger,
                                            GPU::Vulkan::kDefaultAllocationCallbacks);
            g_vkState.debugUtilsMessenger = VK_NULL_HANDLE;
        }
    }

} // namespace

//==============================================================================
//
// Instance Initialization
//
//==============================================================================

namespace {

    std::span<const VkExtensionProperties> GetAvailableInstanceExtensions(const char* layerName)
    {
        u32 count = 0;
        EXPECT_VK(vkEnumerateInstanceExtensionProperties(layerName, &count, nullptr));
        if (!count) {
            return {};
        }
        auto extensions = Memory::ScratchNewArrayUninitialized<VkExtensionProperties>(count);
        EXPECT_VK(vkEnumerateInstanceExtensionProperties(layerName, &count, extensions.data()));
        return {extensions.data(), count};
    }

    std::span<const char* const>
    GetRequiredInstanceExtensions(std::span<const VkExtensionProperties> availableExtensions)
    {
        auto displayExtensions = Display::Vulkan::GetRequiredInstanceExtensions();
        std::span<const char*> extensions{};
        u32 count;

        auto add = [&](const char* extension) {
            if (!extensions.empty()) {
                extensions[count] = extension;
            }
            count++;
        };

        auto gather = [&] {
            count = 0;

            for (const char* extension : displayExtensions) {
                add(extension);
            }

            if (s_startupInfo.debugUtilsRequested) {
                if (CheckExtension(availableExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
                    add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                } else {
                    LOG_WARN(VK_EXT_DEBUG_UTILS_EXTENSION_NAME " extension not available");
                    s_startupInfo.debugUtilsRequested = false;
                }
            }
        };

        gather();
        if (!count) {
            return {};
        }
        extensions = Memory::ScratchNewArrayUninitialized<const char*>(count);
        gather();
        return extensions;
    }

    std::span<const VkLayerProperties> GetAvailableInstanceLayers()
    {
        u32 count = 0;
        EXPECT_VK(vkEnumerateInstanceLayerProperties(&count, nullptr));
        if (!count) {
            return {};
        }
        auto layers = Memory::ScratchNewArrayUninitialized<VkLayerProperties>(count);
        EXPECT_VK(vkEnumerateInstanceLayerProperties(&count, layers.data()));
        return {layers.data(), count};
    }

    void CreateInstance()
    {
        if (g_vkState.instance != VK_NULL_HANDLE) {
            return;
        }

        LOG_DEBUG("Creating Vulkan instance");

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = GAME_TITLE;
        appInfo.apiVersion = GPU::Vulkan::kMinApiVersion;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Query instance extensions.
        auto availableExtensions = GetAvailableInstanceExtensions(nullptr);
        auto requiredExtensions = GetRequiredInstanceExtensions(availableExtensions);

        LOG_DEBUG("Available instance extensions: {}", FormatWrapper{availableExtensions});
        LOG_DEBUG("Requested instance extensions: {}", FormatWrapper{requiredExtensions});

        for (const char* extension : requiredExtensions) {
            if (!CheckExtension(availableExtensions, extension)) {
                FATAL("Missing required Vulkan instance extension: %s", extension);
            }
        }

        createInfo.enabledExtensionCount = static_cast<u32>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        // Enable debug utils if requested and available.
        VkDebugUtilsMessengerCreateInfoEXT debugUtilsInfo;
        if (s_startupInfo.debugUtilsRequested) {
            debugUtilsInfo = MakeDebugUtilsMessengerCreateInfo();
            createInfo.pNext = &debugUtilsInfo;
        }

        // Enable validation layer if requested and available.
        auto availableLayers = GetAvailableInstanceLayers();
        LOG_DEBUG("Available instance layers: {}", FormatWrapper{availableLayers});

        if (s_startupInfo.validationRequested) {
            if (CheckLayer(availableLayers, kValidationLayerName)) {
                LOG_DEBUG("Requested instance layer: {}", kValidationLayerName);
                createInfo.enabledLayerCount = 1;
                createInfo.ppEnabledLayerNames = &kValidationLayerName;
            } else {
                LOG_WARN("{} layer not available", kValidationLayerName);
            }
        }

        // Finally create the instance.
        EXPECT_VK(vkCreateInstance(&createInfo, GPU::Vulkan::kDefaultAllocationCallbacks, &g_vkState.instance));
    }

    void DestroyInstance()
    {
        if (g_vkState.instance != VK_NULL_HANDLE) {
            LOG_DEBUG("Destroying Vulkan instance");
            vkDestroyInstance(g_vkState.instance, GPU::Vulkan::kDefaultAllocationCallbacks);
            g_vkState.instance = VK_NULL_HANDLE;
        }
    }

} // namespace

//==============================================================================
//
// Public API
//
//==============================================================================

void GPU::InitGPU()
{
    LOG_INFO("Initializing Vulkan renderer");
    volkInitialize();
    CheckDebugEnvironment();
    CreateInstance();
    volkLoadInstanceOnly(g_vkState.instance);
    CreateDebugUtilsMessenger();
}

void GPU::ShutDownGPU()
{
    LOG_INFO("Shutting down Vulkan renderer");
    DestroyDebugUtilsMessenger();
    DestroyInstance();
}
