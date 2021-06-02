// faster-utf8-validator
//
// Copyright (c) 2019 Zach Wegner
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// https://github.com/zwegner/faster-utf8-validator

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <intrin.h>

#define GET_UTF8(val, GET_BYTE, ERROR)\
    val= (GET_BYTE);\
    {\
        uint32_t top = (val & 128) >> 1;\
        if ((val & 0xc0) == 0x80 || val >= 0xFE)\
            {ERROR}\
        while (val & top) {\
            unsigned int tmp = (GET_BYTE) - 128;\
            if(tmp>>6)\
                {ERROR}\
            val= (val<<6) + tmp;\
            top <<= 5;\
        }\
        val &= (top << 1) - 1;\
    }
    
typedef enum _cpuid_register
{
    eax = 0,
    ebx = 1,
    ecx = 2,
    edx = 3
}cpuid_register;

static inline bool
has_cpuid_bits(unsigned int level, cpuid_register reg, unsigned int bits)
{
    // Check that the level in question is supported.
    int regs[4];
    __cpuid(regs, level & 0x80000000u);
    if ((unsigned)(regs[0]) < level)
    {
        return false;
    }
    // "The __cpuid intrinsic clears the ECX register before calling the cpuid
    // instruction."
    __cpuid(regs, level);
    return ((unsigned)(regs[reg]) & bits) == bits;
}

static inline bool
cpu_has_avx(void)
{
    const unsigned AVX = 1u << 28;
    const unsigned OSXSAVE = 1u << 27;
    const unsigned XSAVE = 1u << 26;

    const unsigned XMM_STATE = 1u << 1;
    const unsigned YMM_STATE = 1u << 2;
    const unsigned AVX_STATE = XMM_STATE | YMM_STATE;

    return has_cpuid_bits(1u, ecx, AVX | OSXSAVE | XSAVE) &&
           // ensure the OS supports XSAVE of YMM registers
           (_xgetbv(0) & AVX_STATE) == AVX_STATE;
}

static inline bool
cpu_has_avx2(void)
{
    return cpu_has_avx() && has_cpuid_bits(7u, ebx, (1u << 5));
}

static inline bool
cpu_has_sse4_1(void)
{
    return has_cpuid_bits(1u, ecx, (1u << 19));
}

int
z_validate_vec_avx2(__m256i bytes, __m256i shifted_bytes, uint32_t *last_cont)
{
    // Error lookup tables for the first, second, and third nibbles
    // Simple macro to make a vector lookup table for use with vpshufb. Since
    // AVX2 is two 16-byte halves, we duplicate the input values.
#define V_TABLE_16(...) _mm256_setr_epi8(__VA_ARGS__, __VA_ARGS__)
    const __m256i error_1 = V_TABLE_16(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x38);
    const __m256i error_2 = V_TABLE_16(0x0B, 0x01, 0x00, 0x00, 0x10, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x24, 0x20, 0x20);
    const __m256i error_3 = V_TABLE_16(0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x2B, 0x33, 0x35, 0x35, 0x31, 0x31, 0x31, 0x31);
#undef V_TABLE_16

    // Quick skip for ascii-only input. If there are no bytes with the high bit
    // set, we don't need to do any more work. We return either valid or
    // invalid based on whether we expected any continuation bytes here.
    const uint32_t high = _mm256_movemask_epi8(bytes);
    if (!high)
    {
        return *last_cont == 0;
    }

    // Which bytes are required to be continuation bytes
    uint64_t req = *last_cont;

    // Compute the continuation byte mask by finding bytes that start with
    // 11x, 111x, and 1111. For each of these prefixes, we get a bitmask
    // and shift it forward by 1, 2, or 3. This loop should be unrolled by
    // the compiler, and the (n == 1) branch inside eliminated.
    uint32_t set = high;
    set &= _mm256_movemask_epi8(_mm256_slli_epi16(bytes, 1));
    // A bitmask of the actual continuation bytes in the input
    // Mark continuation bytes: those that have the high bit set but
    // not the next one
    const uint32_t cont = high ^ set;

    // We add the shifted mask here instead of ORing it, which would
    // be the more natural operation, so that this line can be done
    // with one lea. While adding could give a different result due
    // to carries, this will only happen for invalid UTF-8 sequences,
    // and in a way that won't cause it to pass validation. Reasoning:
    // Any bits for required continuation bytes come after the bits
    // for their leader bytes, and are all contiguous. For a carry to
    // happen, two of these bit sequences would have to overlap. If
    // this is the case, there is a leader byte before the second set
    // of required continuation bytes (and thus before the bit that
    // will be cleared by a carry). This leader byte will not be
    // in the continuation mask, despite being required. QEDish.
    req += (uint64_t) set << 1;
    set &= _mm256_movemask_epi8(_mm256_slli_epi16(bytes, 2));
    req += (uint64_t) set << 2;
    set &= _mm256_movemask_epi8(_mm256_slli_epi16(bytes, 3));
    req += (uint64_t) set << 3;

    // Check that continuation bytes match. We must cast req from uint64_t
    // (which holds the carry mask in the upper half) to uint32_t, which
    // zeroes out the upper bits
    if (cont != (uint32_t) req)
    {
        return 0;
    }

    // Look up error masks for three consecutive nibbles.
    const __m256i nibbles = _mm256_set1_epi8(0x0F);
    __m256i e_1 = _mm256_shuffle_epi8(error_1, _mm256_and_si256(_mm256_srli_epi16(shifted_bytes, 4), nibbles));
    __m256i e_2 = _mm256_shuffle_epi8(error_2, _mm256_and_si256(shifted_bytes, nibbles));
    __m256i e_3 = _mm256_shuffle_epi8(error_3, _mm256_and_si256(_mm256_srli_epi16(bytes, 4), nibbles));

    // Check if any bits are set in all three error masks
    if (!_mm256_testz_si256(_mm256_and_si256(e_1, e_2), e_3))
    {
        return 0;
    }

    // Save continuation bits and input bytes for the next round
    *last_cont = req >> sizeof(__m256i);
    return 1;
}

static inline int
z_validate_utf8_avx2(const char *data, uint32_t len)
{
    // Keep continuation bits from the previous iteration that carry over to
    // each input chunk vector
    uint32_t last_cont = 0;

    uint32_t offset = 0;
    // Deal with the input up until the last section of bytes
    if (len >= sizeof(__m256i))
    {
        // We need a vector of the input byte stream shifted forward one byte.
        // Since we don't want to read the memory before the data pointer
        // (which might not even be mapped), for the first chunk of input just
        // use vector instructions.
        __m256i shifted_bytes = _mm256_loadu_si256((__m256i *) data);
        //__m256i shl_16 = _mm256_permute2x128_si256(shifted_bytes, _mm256_setzero_si256(), 0x03);
        // shifted_bytes = _mm256_alignr_epi8(shifted_bytes, shl_16, 15);
        shifted_bytes = _mm256_slli_si256(shifted_bytes, 1);

        // Loop over input in sizeof(__m256i)-byte chunks, as long as we can safely read
        // that far into memory
        for (; offset + sizeof(__m256i) < len; offset += sizeof(__m256i))
        {
            __m256i bytes = _mm256_loadu_si256((__m256i *) (data + offset));
            if (!z_validate_vec_avx2(bytes, shifted_bytes, &last_cont))
            {
                return 0;
            }
            shifted_bytes = _mm256_loadu_si256((__m256i *) (data + offset + sizeof(__m256i) - 1));
        }
    }

    // Deal with any bytes remaining. Rather than making a separate scalar path,
    // just fill in a buffer, reading bytes only up to len, and load from that.
    if (offset < len)
    {
        uint8_t buffer[2 * sizeof(__m256i)] = { 0 };

        if (offset != 0)
        {
            buffer[0] = data[offset - 1];
        }
        __movsb(buffer + 1, (const uint8_t *) (data + offset), len - offset);

        __m256i shifted_bytes = _mm256_load_si256((__m256i *) buffer);
        __m256i bytes = _mm256_loadu_si256((__m256i *) (buffer + 1));
        if (!z_validate_vec_avx2(bytes, shifted_bytes, &last_cont))
        {
            return 0;
        }
    }

    // The input is valid if we don't have any more expected continuation bytes
    return last_cont == 0;
}

static int
z_validate_vec_sse4(__m128i bytes, __m128i shifted_bytes, uint32_t *last_cont)
{
    // Error lookup tables for the first, second, and third nibbles
    const __m128i error_1 = _mm_setr_epi8(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x38);
    const __m128i error_2 = _mm_setr_epi8(0x0B, 0x01, 0x00, 0x00, 0x10, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x24, 0x20, 0x20);
    const __m128i error_3 = _mm_setr_epi8(0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x2B, 0x33, 0x35, 0x35, 0x31, 0x31, 0x31, 0x31);

    // Quick skip for ascii-only input. If there are no bytes with the high bit
    // set, we don't need to do any more work. We return either valid or
    // invalid based on whether we expected any continuation bytes here.
    const uint32_t high = _mm_movemask_epi8(bytes);
    if (!high)
    {
        return *last_cont == 0;
    }

    // Which bytes are required to be continuation bytes
    uint32_t req = *last_cont;

    // Compute the continuation byte mask by finding bytes that start with
    // 11x, 111x, and 1111. For each of these prefixes, we get a bitmask
    // and shift it forward by 1, 2, or 3. This loop should be unrolled by
    // the compiler, and the (n == 1) branch inside eliminated.
    uint32_t set = high;
    set &= _mm_movemask_epi8(_mm_slli_epi16(bytes, 1));
    // A bitmask of the actual continuation bytes in the input
    // Mark continuation bytes: those that have the high bit set but
    // not the next one
    const uint32_t cont = high ^ set;
    // We add the shifted mask here instead of ORing it, which would
    // be the more natural operation, so that this line can be done
    // with one lea. While adding could give a different result due
    // to carries, this will only happen for invalid UTF-8 sequences,
    // and in a way that won't cause it to pass validation. Reasoning:
    // Any bits for required continuation bytes come after the bits
    // for their leader bytes, and are all contiguous. For a carry to
    // happen, two of these bit sequences would have to overlap. If
    // this is the case, there is a leader byte before the second set
    // of required continuation bytes (and thus before the bit that
    // will be cleared by a carry). This leader byte will not be
    // in the continuation mask, despite being required. QEDish.
    req += set << 1;
    set &= _mm_movemask_epi8(_mm_slli_epi16(bytes, 2));
    req += set << 2;
    set &= _mm_movemask_epi8(_mm_slli_epi16(bytes, 3));
    req += set << 3;

    // Check that continuation bytes match. We must cast req from uint32_t
    // (which holds the carry mask in the upper half) to uint16_t, which
    // zeroes out the upper bits
    if (cont != (uint16_t) req)
    {
        return 0;
    }

    // Look up error masks for three consecutive nibbles.
    const __m128i nibbles = _mm_set1_epi8(0x0F);
    __m128i e_1 = _mm_shuffle_epi8(error_1, _mm_and_si128(_mm_srli_epi16(shifted_bytes, 4), nibbles));
    __m128i e_2 = _mm_shuffle_epi8(error_2, _mm_and_si128(shifted_bytes, nibbles));
    __m128i e_3 = _mm_shuffle_epi8(error_3, _mm_and_si128(_mm_srli_epi16(bytes, 4), nibbles));

    // Check if any bits are set in all three error masks
    if (!_mm_test_all_zeros(_mm_and_si128(e_1, e_2), e_3))
    {
        return 0;
    }

    // Save continuation bits and input bytes for the next round
    *last_cont = req >> sizeof(__m128i);
    return 1;
}

static inline bool
z_validate_utf8_sse4(const char *data, uint32_t len)
{
    // Keep continuation bits from the previous iteration that carry over to
    // each input chunk vector
    uint32_t last_cont = 0;

    uint32_t offset = 0;
    // Deal with the input up until the last section of bytes
    if (len >= sizeof(__m128i))
    {
        // We need a vector of the input byte stream shifted forward one byte.
        // Since we don't want to read the memory before the data pointer
        // (which might not even be mapped), for the first chunk of input just
        // use vector instructions.
        __m128i shifted_bytes = _mm_loadu_si128((__m128i *) data);
        // shifted_bytes = _mm_alignr_epi8(shifted_bytes, _mm_setzero_si128(), 15);
        shifted_bytes = _mm_slli_si128(shifted_bytes, 1);

        // Loop over input in sizeof(__m128i)-byte chunks, as long as we can safely read
        // that far into memory
        for (; offset + sizeof(__m128i) < len; offset += sizeof(__m128i))
        {
            __m128i bytes = _mm_loadu_si128((__m128i *) (data + offset));
            if (!z_validate_vec_sse4(bytes, shifted_bytes, &last_cont))
            {
                return 0;
            }
            shifted_bytes = _mm_loadu_si128((__m128i *) (data + offset + sizeof(__m128i) - 1));
        }
    }

    // Deal with any bytes remaining. Rather than making a separate scalar path,
    // just fill in a buffer, reading bytes only up to len, and load from that.
    if (offset < len)
    {
        uint8_t buffer[2 * sizeof(__m128i)] = { 0 };

        if (offset != 0)
        {
            buffer[0] = data[offset - 1];
        }
        __movsb(buffer + 1, (const uint8_t *) (data + offset), len - offset);

        __m128i shifted_bytes = _mm_load_si128((__m128i *) (buffer));
        __m128i bytes = _mm_loadu_si128((__m128i *) (buffer + 1));
        if (!z_validate_vec_sse4(bytes, shifted_bytes, &last_cont))
        {
            return 0;
        }
    }

    // The input is valid if we don't have any more expected continuation bytes
    return last_cont == 0;
}

static bool
check_utf8(const uint8_t *str)
{
    const uint8_t *byte;
    uint32_t codepoint, min;
    while (*str)
    {
        byte = str;
        if ((*str >> 7) == 0)
        {
            ++str;
            continue;
        }
        GET_UTF8(codepoint, *(byte++), return false;);
        min = byte - str == 1 ? 0 : byte - str == 2 ? 0x80 : 1 << (5 * (byte - str) - 4);
        if (codepoint < min || codepoint >= 0x110000 || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
        {
            return false;
        }
        str = byte;
    }
    return true;
}

bool WINAPI
on_encoding_validate_utf8(const char *data, size_t len)
{
    if (cpu_has_avx2())
    {
        return z_validate_utf8_avx2(data, (uint32_t) len);
    }
    else if (cpu_has_sse4_1())
    {
        return z_validate_utf8_sse4(data, (uint32_t) len);
    }
    else
    {
        return check_utf8((const uint8_t *) data);
    }
}
