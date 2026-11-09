// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <stdlib.h>

#include <new>

#include <EASTL/allocator.h>

#include "Logging.h"
#include "Math.h"
#include "Memory.h"
#include "OS.h"

//==============================================================================
//
// Misc Utilities
//
//==============================================================================

namespace {

    constexpr uintptr_t AlignUp(uintptr_t value, size_t alignment)
    {
        return ((value - 1) | (alignment - 1)) + 1;
    }

    template <typename T>
    constexpr T* AlignUp(T* ptr, size_t alignment)
    {
        return reinterpret_cast<T*>(AlignUp(reinterpret_cast<uintptr_t>(ptr), alignment));
    }

    [[maybe_unused]] constexpr bool IsPowerOf2(size_t value)
    {
        return value && (value & (value - 1)) == 0;
    }

} // namespace

//==============================================================================
//
// Standard Heap Allocator
//
//==============================================================================

void* Memory::HeapAlloc(size_t size)
{
    void* block = TryHeapAlloc(size);
    if (!block) {
        Logging::Log(Logging::Level::kFatal, nullptr, "Heap allocation failed: size={}", size);
        ::abort();
    }
    return block;
}

void* Memory::HeapAllocAligned(size_t size, size_t alignment)
{
    void* block = TryHeapAllocAligned(size, alignment);
    if (!block) {
        Logging::Log(Logging::Level::kFatal, nullptr, "Aligned heap allocation failed: size={}, alignment={}", size,
                     alignment);
        ::abort();
    }
    return block;
}

void* Memory::TryHeapAlloc(size_t size)
{
    return ::malloc(size);
}

void* Memory::TryHeapAllocAligned(size_t size, size_t alignment)
{
#ifdef _WIN32
    return ::_aligned_malloc(size, alignment);
#else
    return ::aligned_alloc(alignment, size);
#endif
}

void Memory::HeapFree(void* block)
{
    ::free(block);
}

void Memory::HeapFreeAligned(void* block)
{
#ifdef _WIN32
    ::_aligned_free(block);
#else
    ::free(block);
#endif
}

//==============================================================================
//
// Scratch Allocator
//
//==============================================================================

namespace {

    struct ScratchChunk {
        static constexpr size_t kDefaultGrossSize = 1_MiB;

        char* pos;
        char* end;
        ScratchChunk* next;

        static ScratchChunk* Allocate(size_t sizeRequest, size_t alignmentRequest);
        static void Free(ScratchChunk* chunk);
        void* TryReserveBlock(size_t size, size_t alignment);
    };

    thread_local ScratchChunk* tls_scratchHead = nullptr;

    ScratchChunk* ScratchChunk::Allocate(size_t sizeRequest, size_t alignmentRequest)
    {
        if (sizeRequest > SIZE_MAX - sizeof(ScratchChunk)) {
            Logging::Log(Logging::Level::kFatal, nullptr,
                         "Scratch chunk allocation size overflow: sizeRequest={} alignmentRequest={}", sizeRequest,
                         alignmentRequest);
            ::abort();
        }

        size_t alignment = Math::Max(alignmentRequest, alignof(ScratchChunk));
        size_t grossSize = Math::Max(sizeof(ScratchChunk) + sizeRequest, kDefaultGrossSize);
        void* block = Memory::TryHeapAllocAligned(grossSize, alignment);
        if (!block) {
            Logging::Log(Logging::Level::kFatal, nullptr, "Scratch chunk allocation failed: grossSize={}, alignment={}",
                         grossSize, alignment);
            ::abort();
        }

        LOG_DEBUG("Allocated scratch overflow chunk: thread={} addr={} size={} alignment={}",
                  OS::GetCurrentThreadDebugName(), block, grossSize, alignment);

        ScratchChunk* chunk = static_cast<ScratchChunk*>(block);
        *chunk = ScratchChunk{
            .pos = reinterpret_cast<char*>(chunk + 1),
            .end = reinterpret_cast<char*>(chunk) + grossSize,
            .next = nullptr,
        };

        // Link the new chunk into the thread-local list.
        ScratchChunk* prev = tls_scratchHead;
        while (prev->next) {
            prev = prev->next;
        }
        prev->next = chunk;

        return chunk;
    }

    void ScratchChunk::Free(ScratchChunk* chunk)
    {
        LOG_DEBUG("Freeing scratch overflow chunk: thread={} addr={} size={}", OS::GetCurrentThreadDebugName(),
                  static_cast<void*>(chunk), static_cast<size_t>(chunk->end - reinterpret_cast<char*>(chunk)));

        Memory::HeapFreeAligned(chunk);
    }

    void* ScratchChunk::TryReserveBlock(size_t size, size_t alignment)
    {
        char* alignedPos = AlignUp(pos, alignment);
        if (!alignedPos || alignedPos > end || static_cast<size_t>(end - alignedPos) < size) {
            return nullptr;
        }
        pos = alignedPos + size;
        return alignedPos;
    }

} // namespace

void Memory::InitThreadScratchBuffer(void* buffer, size_t size)
{
    ASSERT(size > sizeof(ScratchChunk));
    ASSERT(tls_scratchHead == nullptr);

    LOG_DEBUG("Initializing scratch buffer: thread={} addr={} size={}", OS::GetCurrentThreadDebugName(), buffer, size);

    tls_scratchHead = static_cast<ScratchChunk*>(buffer);
    *tls_scratchHead = {
        .pos = reinterpret_cast<char*>(tls_scratchHead + 1),
        .end = reinterpret_cast<char*>(tls_scratchHead) + size,
        .next = nullptr,
    };
}

void* Memory::ScratchAlloc(size_t size)
{
    return ScratchAllocAligned(size, kDefaultAlignment);
}

void* Memory::ScratchAllocAligned(size_t size, size_t alignment)
{
    ASSERT(size > 0);
    ASSERT(IsPowerOf2(alignment));
    ASSERT(tls_scratchHead != nullptr);

    if (!size) {
        return nullptr;
    }

    // Find an existing chunk with enough free space.
    for (ScratchChunk* chunk = tls_scratchHead; chunk; chunk = chunk->next) {
        void* block = chunk->TryReserveBlock(size, alignment);
        if (block) {
            return block;
        }
    }

    // No existing chunk had enough space. Allocate a new scratch chunk.
    ScratchChunk* chunk = ScratchChunk::Allocate(size, alignment);
    void* block = chunk->TryReserveBlock(size, alignment);
    ASSERT(block != nullptr);
    return block;
}

void* Memory::ScratchAllocArray(size_t count, size_t size)
{
    return ScratchAllocArrayAligned(count, size, kDefaultAlignment);
}

void* Memory::ScratchAllocArrayAligned(size_t count, size_t size, size_t alignment)
{
    if (size && count > SIZE_MAX / size) {
        Logging::Log(Logging::Level::kFatal, nullptr, "Scratch memory block size overflow: count={} size={}", count,
                     size);
    }
    return ScratchAllocAligned(count * size, alignment);
}

void Memory::ScratchReset()
{
    ASSERT(tls_scratchHead != nullptr);

    // Free all overflow chunks.
    ScratchChunk* chunk = tls_scratchHead->next;
    while (chunk) {
        ScratchChunk* next = chunk->next;
        ScratchChunk::Free(chunk);
        chunk = next;
    }

    // Reset the head chunk.
    tls_scratchHead->pos = reinterpret_cast<char*>(tls_scratchHead + 1);
    tls_scratchHead->next = nullptr;
}

//==============================================================================
//
// Standard `operator new` Implementations
//
//==============================================================================

void* operator new(size_t size)
{
    return Memory::HeapAlloc(size);
}

void* operator new[](size_t size)
{
    return Memory::HeapAlloc(size);
}

void* operator new(size_t size, std::align_val_t alignment)
{
    return Memory::HeapAllocAligned(size, static_cast<size_t>(alignment));
}

void* operator new[](size_t size, std::align_val_t alignment)
{
    return Memory::HeapAllocAligned(size, static_cast<size_t>(alignment));
}

void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return Memory::TryHeapAlloc(size);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
    return Memory::TryHeapAlloc(size);
}

void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return Memory::TryHeapAllocAligned(size, static_cast<size_t>(alignment));
}

void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return Memory::TryHeapAllocAligned(size, static_cast<size_t>(alignment));
}

//==============================================================================
//
// Standard `operator delete` Implementations
//
//==============================================================================

void operator delete(void* block) noexcept
{
    Memory::HeapFree(block);
}

void operator delete[](void* block) noexcept
{
    Memory::HeapFree(block);
}

void operator delete(void* block, std::align_val_t)
{
    Memory::HeapFreeAligned(block);
}

void operator delete[](void* block, std::align_val_t)
{
    Memory::HeapFreeAligned(block);
}

void operator delete(void* block, size_t) noexcept
{
    Memory::HeapFree(block);
}

void operator delete[](void* block, size_t) noexcept
{
    Memory::HeapFree(block);
}

void operator delete(void* block, size_t, std::align_val_t) noexcept
{
    Memory::HeapFreeAligned(block);
}

void operator delete[](void* block, size_t, std::align_val_t) noexcept
{
    Memory::HeapFreeAligned(block);
}

void operator delete(void* block, const std::nothrow_t&) noexcept
{
    Memory::HeapFree(block);
}

void operator delete[](void* block, const std::nothrow_t&) noexcept
{
    Memory::HeapFree(block);
}

void operator delete(void* block, std::align_val_t, const std::nothrow_t&) noexcept
{
    Memory::HeapFreeAligned(block);
}

void operator delete[](void* block, std::align_val_t, const std::nothrow_t&) noexcept
{
    Memory::HeapFreeAligned(block);
}

//==============================================================================
//
// EASTL Allocation Implementations
//
//==============================================================================

void* operator new[](size_t size, const char*, int, unsigned, const char*, int)
{
    return Memory::HeapAlloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char*, int, unsigned,
                     const char* fileName, int lineNum)
{
    if (alignmentOffset) {
        size_t padding = alignment - alignmentOffset % alignment;
        if (padding) {
            if (size > SIZE_MAX - padding) {
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
            size += padding;
        }
    }
    return Memory::HeapAllocAligned(size, alignment);
}
