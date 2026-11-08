// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#include <Client/Main.h>
#include <Core/Logging.h>
#include <GameDefs.h>

#include "Display.h"

#if GPU_VULKAN
# include "Vulkan.h"
# include <SDL3/SDL_vulkan.h>
#endif

namespace {

    struct DisplayState {
        SDL_Window* window = nullptr;
    };

    DisplayState s_displayState{};

} // namespace

//==============================================================================
//
// Common Display Functions
//
//==============================================================================

void Display::InitDisplay()
{
    if (!s_displayState.window) {
        u32 flags = SDL_WINDOW_RESIZABLE;
#if GPU_VULKAN
        flags |= SDL_WINDOW_VULKAN;
#endif

        LOG_INFO("Creating SDL window");

        s_displayState.window = SDL_CreateWindow(GAME_TITLE, 640, 480, flags);
        if (!s_displayState.window) {
            FATAL("Failed to create SDL window: {}", SDL_GetError());
        }
    }
}

void Display::ShutDownDisplay()
{
    if (s_displayState.window) {
        LOG_INFO("Destroying SDL window");
        SDL_DestroyWindow(s_displayState.window);
        s_displayState.window = nullptr;
    }
}

void Display::PollEvents()
{
    SDL_Event event;

    while (!Client::IsQuitting() && SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                Client::Quit();
                break;
            default:
                break;
        }
    }
}

//==============================================================================
//
// Vulkan Support Functions
//
//==============================================================================

#if GPU_VULKAN

std::span<const char* const> Display::Vulkan::GetRequiredInstanceExtensions()
{
    u32 count = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    return {extensions, count};
}

VkSurfaceKHR Display::Vulkan::CreateSurface(VkInstance instance, const VkAllocationCallbacks* allocator)
{
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(s_displayState.window, instance, allocator, &surface)) {
        FATAL("Failed to create Vulkan surface: {}", SDL_GetError());
    }
    return surface;
}

void Display::Vulkan::DestroySurface(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* allocator)
{
    SDL_Vulkan_DestroySurface(instance, surface, allocator);
}

#endif // GPU_VULKAN
