// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#ifdef _WIN32
# include <windows.h>
#endif

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include <Core/Logging.h>
#include <Display/Display.h>

#include "Main.h"

namespace {

    struct ClientState {
        bool quitRequested = false;
    };

    ClientState s_clientState{};

} // namespace

//==============================================================================
//
// Public API
//
//==============================================================================

void Client::Quit()
{
    s_clientState.quitRequested = true;
}

bool Client::IsQuitting()
{
    return s_clientState.quitRequested;
}

//==============================================================================
//
// Main Loop
//
//==============================================================================

namespace {

    void MainLoop()
    {
        while (!s_clientState.quitRequested) {
            Display::PollEvents();
            if (s_clientState.quitRequested) {
                break;
            }

            // No vsync yet; avoid busy-waiting
            SDL_Delay(10);
        }
    }

} // namespace

//==============================================================================
//
// Entry Point
//
//==============================================================================

namespace {

    int ClientMain(int, OSChar**)
    {
        Logging::InitLogger();

        LOG_INFO("Initializing...");
        Display::InitDisplay();

        LOG_INFO("Game started!");
        MainLoop();

        LOG_INFO("Shutting down...");
        Display::ShutDownDisplay();
        SDL_Quit();
        Logging::ShutDownLogger();

        return 0;
    }

} // namespace

#ifdef _WIN32

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    return ClientMain(__argc, __wargv);
}

#else

int main(int argc, char** argv)
{
    return ClientMain(argc, argv);
}

#endif
