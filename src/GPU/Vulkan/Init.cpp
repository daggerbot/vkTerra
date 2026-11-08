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
// Surface Initialization
//
//==============================================================================

namespace {

    void CreateSurface()
    {
        if (!g_vkState.surface) {
            LOG_DEBUG("Creating surface");
            g_vkState.surface =
                Display::Vulkan::CreateSurface(g_vkState.instance, GPU::Vulkan::kDefaultAllocationCallbacks);
        }
    }

    void DestroySurface()
    {
        if (g_vkState.surface) {
            LOG_DEBUG("Destroying surface");
            Display::Vulkan::DestroySurface(g_vkState.instance, g_vkState.surface,
                                            GPU::Vulkan::kDefaultAllocationCallbacks);
            g_vkState.surface = VK_NULL_HANDLE;
        }
    }

} // namespace

//==============================================================================
//
// Physical Device Selection
//
//==============================================================================

namespace {

    struct PhysicalDeviceInfo {
        VkPhysicalDevice handle;
        VkPhysicalDeviceProperties properties;
        std::span<VkQueueFamilyProperties> queueFamilies;
        i32 graphicsQueueFamilyIndex = -1;
        i32 presentationQueueFamilyIndex = -1;
    };

    std::span<VkPhysicalDevice> GetAvailablePhysicalDevices()
    {
        u32 count = 0;
        EXPECT_VK(vkEnumeratePhysicalDevices(g_vkState.instance, &count, nullptr));
        if (!count) {
            return {};
        }
        auto devices = Memory::ScratchNewArrayUninitialized<VkPhysicalDevice>(count);
        EXPECT_VK(vkEnumeratePhysicalDevices(g_vkState.instance, &count, devices.data()));
        return {devices.data(), count};
    }

    void ChooseQueueFamilies(PhysicalDeviceInfo& info)
    {
        i32 bestGraphicsIndex = -1;
        i32 bestPresentationIndex = -1;

        for (size_t i = 0; i < info.queueFamilies.size(); i++) {
            bool supportsGraphics = info.queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
            VkBool32 supportsPresentation = 0;
            EXPECT_VK(vkGetPhysicalDeviceSurfaceSupportKHR(info.handle, static_cast<u32>(i), g_vkState.surface,
                                                           &supportsPresentation));

            if (supportsGraphics && supportsPresentation) {
                // Prefer a queue family that supports all required capabilities.
                bestGraphicsIndex = static_cast<i32>(i);
                bestPresentationIndex = static_cast<i32>(i);
                break;
            }
            if (supportsGraphics && bestGraphicsIndex == -1) {
                bestGraphicsIndex = static_cast<i32>(i);
            }
            if (supportsPresentation && bestPresentationIndex == -1) {
                bestPresentationIndex = static_cast<i32>(i);
            }
        }

        info.graphicsQueueFamilyIndex = bestGraphicsIndex;
        info.presentationQueueFamilyIndex = bestPresentationIndex;
    }

    PhysicalDeviceInfo GetPhysicalDeviceInfo(VkPhysicalDevice device)
    {
        PhysicalDeviceInfo info{};
        info.handle = device;
        vkGetPhysicalDeviceProperties(device, &info.properties);

        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        auto queueFamilies = Memory::ScratchNewArrayUninitialized<VkQueueFamilyProperties>(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        info.queueFamilies = {queueFamilies.data(), queueFamilyCount};

        ChooseQueueFamilies(info);

        return info;
    }

    bool IsPhysicalDeviceSuitable(const PhysicalDeviceInfo& info)
    {
        if (VK_API_VERSION_VARIANT(info.properties.apiVersion) != 0
            || VK_API_VERSION_MAJOR(info.properties.apiVersion) != VK_API_VERSION_MAJOR(GPU::Vulkan::kMinApiVersion)
            || VK_API_VERSION_MINOR(info.properties.apiVersion) < VK_API_VERSION_MINOR(GPU::Vulkan::kMinApiVersion))
        {
            return false;
        }

        if (info.graphicsQueueFamilyIndex == -1 || info.presentationQueueFamilyIndex == -1) {
            return false;
        }

        return true;
    }

    int GetPhysicalDeviceTypeScore(VkPhysicalDeviceType type)
    {
        switch (type) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                return 3;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                return 2;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                return 1;
            default:
                return 0;
        }
    }

    bool IsPhysicalDeviceBetter(const PhysicalDeviceInfo& testInfo, const PhysicalDeviceInfo& controlInfo)
    {
        // Compare by device type.
        int testScore = GetPhysicalDeviceTypeScore(testInfo.properties.deviceType);
        int controlScore = GetPhysicalDeviceTypeScore(controlInfo.properties.deviceType);
        if (testScore > controlScore) {
            return true;
        } else if (testScore < controlScore) {
            return false;
        }

        // Prefer a unified queue family for graphics and presentation.
        testScore = (testInfo.graphicsQueueFamilyIndex == testInfo.presentationQueueFamilyIndex) ? 1 : 0;
        controlScore = (controlInfo.graphicsQueueFamilyIndex == controlInfo.presentationQueueFamilyIndex) ? 1 : 0;
        if (testScore > controlScore) {
            return true;
        } else if (testScore < controlScore) {
            return false;
        }

        return false;
    }

    void ChoosePhysicalDevice()
    {
        auto allDevices = GetAvailablePhysicalDevices();
        bool suitableDeviceFound = false;
        PhysicalDeviceInfo bestDeviceInfo{};

        for (size_t i = 0; i < allDevices.size(); i++) {
            VkPhysicalDevice device = allDevices[i];
            auto info = GetPhysicalDeviceInfo(device);

            LOG_DEBUG("Physical device {}: {}", i, info.properties.deviceName);

            if (IsPhysicalDeviceSuitable(info)) {
                if (suitableDeviceFound) {
                    if (IsPhysicalDeviceBetter(info, bestDeviceInfo)) {
                        bestDeviceInfo = info;
                    }
                } else {
                    bestDeviceInfo = info;
                    suitableDeviceFound = true;
                }
            }
        }

        if (!suitableDeviceFound) {
            FATAL("No suitable Vulkan physical device found");
        }

        LOG_INFO("Using Vulkan physical device: {}", bestDeviceInfo.properties.deviceName);
        LOG_DEBUG("Unified graphics and presentation: {}",
                  bestDeviceInfo.graphicsQueueFamilyIndex == bestDeviceInfo.presentationQueueFamilyIndex);

        g_vkState.physicalDevice = bestDeviceInfo.handle;
        g_vkState.graphicsQueueFamilyIndex = static_cast<u32>(bestDeviceInfo.graphicsQueueFamilyIndex);
        g_vkState.presentationQueueFamilyIndex = static_cast<u32>(bestDeviceInfo.presentationQueueFamilyIndex);
    }

} // namespace

//==============================================================================
//
// Logical Device Initialization
//
//==============================================================================

namespace {

    void CreateDevice()
    {
        static const float kQueuePriority = 1.0f;

        if (g_vkState.device) {
            return;
        }

        LOG_DEBUG("Creating logical device");

        VkDeviceQueueCreateInfo queueCreateInfo[2]{};
        u32 queueFamilyCount = 1;

        queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[0].queueFamilyIndex = g_vkState.graphicsQueueFamilyIndex;
        queueCreateInfo[0].queueCount = 1;
        queueCreateInfo[0].pQueuePriorities = &kQueuePriority;

        if (g_vkState.graphicsQueueFamilyIndex != g_vkState.presentationQueueFamilyIndex) {
            queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo[1].queueFamilyIndex = g_vkState.presentationQueueFamilyIndex;
            queueCreateInfo[1].queueCount = 1;
            queueCreateInfo[1].pQueuePriorities = &kQueuePriority;
            queueFamilyCount = 2;
        }

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = queueFamilyCount;
        createInfo.pQueueCreateInfos = queueCreateInfo;

        EXPECT_VK(vkCreateDevice(g_vkState.physicalDevice, &createInfo, GPU::Vulkan::kDefaultAllocationCallbacks,
                                 &g_vkState.device));
    }

    void DestroyDevice()
    {
        if (g_vkState.device) {
            LOG_DEBUG("Destroying logical device");
            vkDestroyDevice(g_vkState.device, GPU::Vulkan::kDefaultAllocationCallbacks);
            g_vkState.device = VK_NULL_HANDLE;
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
    EXPECT_VK(volkInitialize());
    CheckDebugEnvironment();
    CreateInstance();
    volkLoadInstanceOnly(g_vkState.instance);
    CreateDebugUtilsMessenger();
    CreateSurface();
    ChoosePhysicalDevice();
    CreateDevice();
    volkLoadDevice(g_vkState.device);
}

void GPU::ShutDownGPU()
{
    LOG_INFO("Shutting down Vulkan renderer");
    DestroyDevice();
    g_vkState.physicalDevice = VK_NULL_HANDLE;
    DestroySurface();
    DestroyDebugUtilsMessenger();
    DestroyInstance();
    volkFinalize();
}
