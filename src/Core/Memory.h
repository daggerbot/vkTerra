// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

#include <span>
#include <type_traits>

#include "Concepts.h"

namespace Memory {

    inline constexpr size_t kDefaultAlignment = alignof(max_align_t);

    /// Matches types that are standard layout and trivially destructible, suitable for scratch allocation.
    template <typename T>
    concept Scratchable = std::is_standard_layout_v<T> && std::is_trivially_destructible_v<T>;

    // Standard heap allocator functions.
    void* HeapAlloc(size_t size);
    void* HeapAllocAligned(size_t size, size_t alignment);
    void* TryHeapAlloc(size_t size);
    void* TryHeapAllocAligned(size_t size, size_t alignment);
    void HeapFree(void* block);
    void HeapFreeAligned(void* block);

    // Thread-local scratch allocator functions.
    // `InitThreadScratchBuffer` must be called before any scratch allocation functions are used on a given thread.
    void InitThreadScratchBuffer(void* buffer, size_t size);
    void* ScratchAlloc(size_t size);
    void* ScratchAllocAligned(size_t size, size_t alignment);
    void* ScratchAllocArray(size_t count, size_t size);
    void* ScratchAllocArrayAligned(size_t count, size_t size, size_t alignment);
    void ScratchReset();

    /// Allocates and constructs an object of type `T` in the scratch buffer.
    template <Scratchable T, typename... Args>
    T* ScratchNew(Args&&... args)
    requires(std::is_constructible_v<T, decltype(std::forward<Args>(args))...>)
    {
        T* obj = static_cast<T*>(ScratchAllocAligned(sizeof(T), alignof(T)));
        new (obj) T{std::forward<Args>(args)...};
        return obj;
    }

    /// Allocates an uninitialized object of type `T` in the scratch buffer.
    template <Scratchable T>
    T* ScratchNewUninitialized()
    {
        return static_cast<T*>(ScratchAllocAligned(sizeof(T), alignof(T)));
    }

    /// Allocates and constructs an array of objects of type `T` in the scratch buffer.
    template <Scratchable T, typename... Args>
    std::span<T> ScratchNewArray(size_t count, Args... args)
    requires(std::is_constructible_v<T, Args...>
             && ((std::is_lvalue_reference_v<Args> && ...)
                 || (std::is_trivially_copyable_v<std::remove_cvref_t<Args>> && ...)))
    {
        T* array = static_cast<T*>(ScratchAllocArrayAligned(count, sizeof(T), alignof(T)));
        for (size_t i = 0; i < count; ++i) {
            new (array + i) T{args...};
        }
        return std::span<T>{array, count};
    }

    /// Allocates an uninitialized array of objects of type `T` in the scratch buffer.
    template <Scratchable T>
    std::span<T> ScratchNewArrayUninitialized(size_t count)
    {
        T* array = static_cast<T*>(ScratchAllocArrayAligned(count, sizeof(T), alignof(T)));
        return std::span<T>{array, count};
    }

} // namespace Memory
