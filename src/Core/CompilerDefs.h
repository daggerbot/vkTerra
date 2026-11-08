// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#pragma once

/// \def ALWAYS_INLINE
/// Attribute that hints to the compiler to always inline the function.
#if defined(__GNUC__)
# define ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
# define ALWAYS_INLINE __forceinline
#else
# define ALWAYS_INLINE inline
#endif

/// \def NO_INLINE
/// Attribute that hints to the compiler to never inline the function.
#if defined(__GNUC__)
# define NO_INLINE __attribute__((noinline))
#elif defined(_MSC_VER)
# define NO_INLINE __declspec(noinline)
#else
# define NO_INLINE
#endif

/// \def RESTRICT
/// Attribute that hints to the compiler that the pointer is not aliased.
#if (__STDC_VERSION__ - 0) >= 199901L
# define RESTRICT restrict
#elif defined(__GNUC__) || defined(_MSC_VER)
# define RESTRICT __restrict
#else
# define RESTRICT
#endif
