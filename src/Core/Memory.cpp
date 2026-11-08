// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <stdlib.h>

#include <new>

#include <EASTL/allocator.h>

#include "Logging.h"

//==============================================================================
//
// Main Allocation Functions
//
//==============================================================================

namespace {

    void* TryHeapAlloc(size_t size)
    {
        return ::malloc(size);
    }

    void* HeapAlloc(size_t size)
    {
        void* block = TryHeapAlloc(size);
        if (!block) {
            Logging::Log(Logging::Level::kFatal, nullptr, "Heap allocation failed: size={}", size);
            ::abort();
        }
        return block;
    }

    void* TryHeapAllocAligned(size_t size, size_t alignment)
    {
#ifdef _WIN32
        return ::_aligned_malloc(size, alignment);
#else
        return ::aligned_alloc(alignment, size);
#endif
    }

    void* HeapAllocAligned(size_t size, size_t alignment)
    {
        void* block = TryHeapAllocAligned(size, alignment);
        if (!block) {
            Logging::Log(Logging::Level::kFatal, nullptr, "Aligned heap allocation failed: size={}, alignment={}", size,
                         alignment);
            ::abort();
        }
        return block;
    }

    void HeapFree(void* block)
    {
        ::free(block);
    }

    void HeapFreeAligned(void* block)
    {
#ifdef _WIN32
        ::_aligned_free(block);
#else
        ::free(block);
#endif
    }

} // namespace

//==============================================================================
//
// Standard `operator new` Implementations
//
//==============================================================================

void* operator new(size_t size)
{
    return HeapAlloc(size);
}

void* operator new[](size_t size)
{
    return HeapAlloc(size);
}

void* operator new(size_t size, std::align_val_t alignment)
{
    return HeapAllocAligned(size, static_cast<size_t>(alignment));
}

void* operator new[](size_t size, std::align_val_t alignment)
{
    return HeapAllocAligned(size, static_cast<size_t>(alignment));
}

void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return TryHeapAlloc(size);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
    return TryHeapAlloc(size);
}

void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return TryHeapAllocAligned(size, static_cast<size_t>(alignment));
}

void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return TryHeapAllocAligned(size, static_cast<size_t>(alignment));
}

//==============================================================================
//
// Standard `operator delete` Implementations
//
//==============================================================================

void operator delete(void* block) noexcept
{
    HeapFree(block);
}

void operator delete[](void* block) noexcept
{
    HeapFree(block);
}

void operator delete(void* block, std::align_val_t)
{
    HeapFreeAligned(block);
}

void operator delete[](void* block, std::align_val_t)
{
    HeapFreeAligned(block);
}

void operator delete(void* block, size_t) noexcept
{
    HeapFree(block);
}

void operator delete[](void* block, size_t) noexcept
{
    HeapFree(block);
}

void operator delete(void* block, size_t, std::align_val_t) noexcept
{
    HeapFreeAligned(block);
}

void operator delete[](void* block, size_t, std::align_val_t) noexcept
{
    HeapFreeAligned(block);
}

void operator delete(void* block, const std::nothrow_t&) noexcept
{
    HeapFree(block);
}

void operator delete[](void* block, const std::nothrow_t&) noexcept
{
    HeapFree(block);
}

void operator delete(void* block, std::align_val_t, const std::nothrow_t&) noexcept
{
    HeapFreeAligned(block);
}

void operator delete[](void* block, std::align_val_t, const std::nothrow_t&) noexcept
{
    HeapFreeAligned(block);
}

//==============================================================================
//
// EASTL Allocation Implementations
//
//==============================================================================

void* operator new[](size_t size, const char*, int, unsigned, const char*, int)
{
    return HeapAlloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char*, int, unsigned,
                     const char* fileName, int lineNum)
{
    if (alignmentOffset) {
        alignmentOffset %= alignment;
        if (alignmentOffset) {
            if (size > SIZE_MAX - (alignment - alignmentOffset)) {
                Logging::SourceLocation location{
                    .fileName = fileName,
                    .funcName = nullptr,
                    .lineNum = static_cast<u32>(lineNum),
                };
                Logging::Log(Logging::Level::kFatal, &location,
                             "Memory allocation size overflow due to alignment offset: size={} alignment={} offset={}",
                             size, alignment, alignmentOffset);
                ::abort();
            }
            size += alignment - alignmentOffset;
        }
    }
    return HeapAllocAligned(size, alignment);
}
