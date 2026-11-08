// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <concepts>
#include <type_traits>

#include "Types.h"

namespace Core {

    /// Executes a given function when the scope is exited.
    /// Typically used to defer resource cleanup until the scope is exited.
    template <std::invocable F>
    class ScopeExit {
    public:
        constexpr ScopeExit()
        requires(std::is_default_constructible_v<F>)
        = default;

        ScopeExit(const ScopeExit&) = delete;
        ScopeExit(ScopeExit&& other) = delete;

        constexpr ScopeExit(F&& func)
        requires(std::is_move_constructible_v<F>)
            : m_func{std::move(func)}
        {
        }

        constexpr ~ScopeExit() { m_func(); }

        ScopeExit& operator=(const ScopeExit&) = delete;
        ScopeExit& operator=(ScopeExit&& other) = delete;

    private:
        F m_func{};
    };

} // namespace Core
