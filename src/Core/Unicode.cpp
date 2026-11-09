// Copyright (c) 2026, vkTerra Contributors
// SPDX-License-Identifier: MPL-2.0
// See LICENSE-MPL-2.0.txt for more information.

#include <limits.h>
#include <string.h>

#include <array>

#include <EASTL/unique_ptr.h>

#include <Core/Logging.h>

#include "Unicode.h"

namespace {

    template <typename CharT, size_t N = sizeof(CharT) * CHAR_BIT>
    struct UtfCodec;

    //==============================================================================
    //
    // UTF-8 Codec
    //
    //==============================================================================

    template <typename CharT>
    struct UtfCodec<CharT, 8> {
        using CharType = CharT;

        static bool DecodeSequence(const CharT** inPos, size_t* inRem, u32* outCodePoint)
        {
            if (!*inRem) {
                return false;
            }

            u8 unit = static_cast<u8>(**inPos);
            u8 seqLength;
            u32 codePoint;

            if (!(unit & 0x80)) {
                // 0xxxxxxx (ASCII)
                *outCodePoint = unit;
                ++*inPos;
                --*inRem;
                return true;
            } else if ((unit & 0xE0) == 0xC0) {
                // 110xxxxx 10xxxxxx
                seqLength = 2;
                codePoint = unit & 0x1F;
            } else if ((unit & 0xF0) == 0xE0) {
                // 1110xxxx 10xxxxxx 10xxxxxx
                seqLength = 3;
                codePoint = unit & 0x0F;
            } else if ((unit & 0xF8) == 0xF0) {
                // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                seqLength = 4;
                codePoint = unit & 0x07;
            } else {
                // Invalid leading byte.
                *outCodePoint = Unicode::kReplacementChar;
                ++*inPos;
                --*inRem;
                return true;
            }

            for (u8 i = 1; i < seqLength; i++) {
                if (i >= *inRem) {
                    // Incomplete sequence.
                    return false;
                }
                unit = static_cast<u8>((*inPos)[i]);
                if ((unit & 0xC0) != 0x80) {
                    // Invalid continuation byte.
                    *outCodePoint = Unicode::kReplacementChar;
                    *inPos += i;
                    *inRem -= i;
                    return true;
                }
                codePoint = (codePoint << 6) | (unit & 0x3F);
            }

            if (codePoint > Unicode::kMaxChar) {
                *outCodePoint = Unicode::kReplacementChar;
            } else if (codePoint >= Unicode::kMinSurrogate && codePoint <= Unicode::kMaxSurrogate) {
                *outCodePoint = Unicode::kReplacementChar;
            } else {
                *outCodePoint = codePoint;
            }

            *inPos += seqLength;
            *inRem -= seqLength;
            return true;
        }

        static bool EncodeSequence(u32 codePoint, CharT** outPos, size_t* outRem)
        {
            if (codePoint <= Unicode::kMaxAsciiChar) {
                if (*outRem < 1) {
                    return false;
                }
                *(*outPos)++ = static_cast<CharT>(codePoint);
                *outRem -= 1;
                return true;
            } else if (codePoint <= 0x7FF) {
                if (*outRem < 2) {
                    return false;
                }
                *(*outPos)++ = static_cast<CharT>(0xC0 | (codePoint >> 6));
                *(*outPos)++ = static_cast<CharT>(0x80 | (codePoint & 0x3F));
                *outRem -= 2;
                return true;
            } else if (codePoint <= Unicode::kMaxBmpChar) {
                if (*outRem < 3) {
                    return false;
                } else if (codePoint >= Unicode::kMinSurrogate && codePoint <= Unicode::kMaxSurrogate) {
                    codePoint = Unicode::kReplacementChar;
                }
                *(*outPos)++ = static_cast<CharT>(0xE0 | (codePoint >> 12));
                *(*outPos)++ = static_cast<CharT>(0x80 | ((codePoint >> 6) & 0x3F));
                *(*outPos)++ = static_cast<CharT>(0x80 | (codePoint & 0x3F));
                *outRem -= 3;
                return true;
            } else if (codePoint <= Unicode::kMaxChar) {
                if (*outRem < 4) {
                    return false;
                }
                *(*outPos)++ = static_cast<CharT>(0xF0 | (codePoint >> 18));
                *(*outPos)++ = static_cast<CharT>(0x80 | ((codePoint >> 12) & 0x3F));
                *(*outPos)++ = static_cast<CharT>(0x80 | ((codePoint >> 6) & 0x3F));
                *(*outPos)++ = static_cast<CharT>(0x80 | (codePoint & 0x3F));
                *outRem -= 4;
                return true;
            } else {
                return EncodeSequence(Unicode::kReplacementChar, outPos, outRem);
            }
        }

        static u8 GetSequenceLength(u32 codePoint)
        {
            if (codePoint <= Unicode::kMaxAsciiChar) {
                return 1;
            } else if (codePoint <= 0x7FF) {
                return 2;
            } else if (codePoint <= Unicode::kMaxBmpChar) {
                return 3;
            } else if (codePoint <= Unicode::kMaxChar) {
                return 4;
            } else {
                return 3; // Replacement character length
            }
        }
    };

    //==============================================================================
    //
    // UTF-16 Codec
    //
    //==============================================================================

    template <typename CharT>
    struct UtfCodec<CharT, 16> {
        using CharType = CharT;

        static bool DecodeSequence(const CharT** inPos, size_t* inRem, u32* outCodePoint)
        {
            if (!*inRem) {
                return false;
            }

            u16 unit1 = static_cast<u16>(**inPos);

            if ((unit1 & 0xF800) != 0xD800) {
                // Single char sequence.
                *outCodePoint = unit1;
                ++*inPos;
                --*inRem;
                return true;
            } else if (!(unit1 & 0x400)) {
                // Possible surrogate pair.
                if (*inRem < 2) {
                    // Incomplete sequence.
                    return false;
                }
                ++*inPos;
                --*inRem;
                u16 unit2 = static_cast<u16>(*(*inPos + 1));
                if ((unit2 & 0xFC00) != 0xDC00) {
                    // Unpaired high surrogate.
                    *outCodePoint = Unicode::kReplacementChar;
                    return true;
                }
                ++*inPos;
                --*inRem;
                *outCodePoint = ((static_cast<u32>(unit1 & 0x3FF) << 10) | (unit2 & 0x3FF)) + 0x10000;
                return true;
            } else {
                // Unpaired low surrogate.
                *outCodePoint = Unicode::kReplacementChar;
                ++*inPos;
                --*inRem;
                return true;
            }
        }

        static bool EncodeSequence(u32 codePoint, CharT** outPos, size_t* outRem)
        {
            if (codePoint <= Unicode::kMaxBmpChar) {
                if (*outRem < 1) {
                    return false;
                } else if (codePoint >= Unicode::kMinSurrogate && codePoint <= Unicode::kMaxSurrogate) {
                    codePoint = Unicode::kReplacementChar;
                }
                *(*outPos)++ = static_cast<CharT>(codePoint);
                --*outRem;
                return true;
            } else if (codePoint <= Unicode::kMaxChar) {
                if (*outRem < 2) {
                    return false;
                }
                codePoint -= 0x10000;
                *(*outPos)++ = static_cast<CharT>((codePoint >> 10) | 0xD800);
                *(*outPos)++ = static_cast<CharT>((codePoint & 0x3FF) | 0xDC00);
                *outRem -= 2;
                return true;
            } else {
                return EncodeSequence(Unicode::kReplacementChar, outPos, outRem);
            }
        }

        static u8 GetSequenceLength(u32 codePoint)
        {
            if (codePoint <= Unicode::kMaxBmpChar) {
                return 1;
            } else if (codePoint <= Unicode::kMaxChar) {
                return 2;
            } else {
                return 1; // Replacement character
            }
        }
    };

    //==============================================================================
    //
    // UTF-32 Codec
    //
    //==============================================================================

    template <typename CharT>
    struct UtfCodec<CharT, 32> {
        using CharType = CharT;

        static bool DecodeSequence(const CharT** inPos, size_t* inRem, u32* outCodePoint)
        {
            if (!*inRem) {
                return false;
            }

            u32 codePoint = static_cast<u32>(*(*inPos)++);
            *inRem -= 1;

            if (codePoint > Unicode::kMaxChar) {
                *outCodePoint = Unicode::kReplacementChar;
            } else if (codePoint >= Unicode::kMinSurrogate && codePoint <= Unicode::kMaxSurrogate) {
                *outCodePoint = Unicode::kReplacementChar;
            } else {
                *outCodePoint = codePoint;
            }

            return true;
        }

        static bool EncodeSequence(u32 codePoint, CharT** outPos, size_t* outRem)
        {
            if (!*outRem) {
                return false;
            }

            if (codePoint > Unicode::kMaxChar) {
                codePoint = Unicode::kReplacementChar;
            } else if (codePoint >= Unicode::kMinSurrogate && codePoint <= Unicode::kMaxSurrogate) {
                codePoint = Unicode::kReplacementChar;
            }

            *(*outPos)++ = static_cast<CharT>(codePoint);
            --*outRem;
            return true;
        }

        static u8 GetSequenceLength(u32) { return 1; }
    };

    //==============================================================================
    //
    // Implementation
    //
    //==============================================================================

    template <typename Decoder, typename Encoder>
    void CallWithConvertedString(eastl::basic_string_view<typename Decoder::CharType> inStr,
                                 void (*callback)(void* userData,
                                                  eastl::basic_string_view<typename Encoder::CharType> outStr),
                                 void* userData)
    {
        using InChar = typename Decoder::CharType;
        using OutChar = typename Encoder::CharType;

        const InChar* inPos = inStr.data();
        size_t inRem = inStr.size();
        OutChar stackBuf[512 / sizeof(OutChar)];
        OutChar* outPos = stackBuf;
        size_t outRem = std::size(stackBuf) - 1; // -1 for null terminator
        u32 codePoint;
        size_t stackBufUsed;

        // Try converting into the stack buffer first.
        while (inRem) {
            const InChar* prevInPos = inPos;
            size_t prevInRem = inRem;

            if (!Decoder::DecodeSequence(&inPos, &inRem, &codePoint)) {
                codePoint = Unicode::kReplacementChar;
                inRem = 0;
            }
            if (!Encoder::EncodeSequence(codePoint, &outPos, &outRem)) {
                inPos = prevInPos;
                inRem = prevInRem;
                break;
            }
        }

        // Invoke the callback now if we're already done.
        stackBufUsed = static_cast<size_t>(outPos - stackBuf);
        if (!inRem) {
            *outPos = 0;
            callback(userData, eastl::basic_string_view<OutChar>{stackBuf, stackBufUsed});
            return;
        }

        // Look ahead and determine the length of the fully converted string.
        const InChar* lookaheadPos = inPos;
        size_t lookaheadRem = inRem;
        size_t length = stackBufUsed;
        while (lookaheadRem) {
            if (!Decoder::DecodeSequence(&lookaheadPos, &lookaheadRem, &codePoint)) {
                codePoint = Unicode::kReplacementChar;
                lookaheadRem = 0;
            }
            length += Encoder::GetSequenceLength(codePoint);
        }

        // Allocate a heap buffer and copy our progress into it.
        eastl::unique_ptr<OutChar[]> heapBuf{new OutChar[length + 1]}; // +1 for null terminator
        memcpy(heapBuf.get(), stackBuf, stackBufUsed * sizeof(OutChar));

        // Finish converting the remaining input into the heap buffer.
        outPos = heapBuf.get() + stackBufUsed;
        outRem = length - stackBufUsed;
        while (inRem) {
            if (!Decoder::DecodeSequence(&inPos, &inRem, &codePoint)) {
                codePoint = Unicode::kReplacementChar;
                inRem = 0;
            }
            bool sequenceEncoded = Encoder::EncodeSequence(codePoint, &outPos, &outRem);
            ASSERT(sequenceEncoded);
        }
        ASSERT(outRem == 0);

        // Invoke the callback with the fully converted string.
        *outPos = 0;
        callback(userData, eastl::basic_string_view<OutChar>{heapBuf.get(), length});
    }

} // namespace

void Unicode::CallWithUtf8ToWide(eastl::string_view inStr, void (*callback)(void* userData, eastl::wstring_view outStr),
                                 void* userData)
{
    ::CallWithConvertedString<UtfCodec<char>, UtfCodec<wchar_t>>(inStr, callback, userData);
}

void Unicode::CallWithWideToUtf8(eastl::wstring_view inStr, void (*callback)(void* userData, eastl::string_view outStr),
                                 void* userData)
{
    ::CallWithConvertedString<UtfCodec<wchar_t>, UtfCodec<char>>(inStr, callback, userData);
}
