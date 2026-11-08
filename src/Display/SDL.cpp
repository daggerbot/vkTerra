// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#include <Client/Main.h>
#include <Core/Logging.h>
#include <GameDefs.h>

#include "Display.h"

namespace {

    struct DisplayState {
        SDL_Window* window = nullptr;
    };

    DisplayState s_displayState{};

} // namespace

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
