#ifndef KECCAK_HPP
#define KECCAK_HPP
/* ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
 * Copyright 2018 Pawel Bylica.
 * Copyright 2022-2023 The Sparq Developers.
 * Licensed under the Apache License, Version 2.0. See the LICENSE file.
 */

#include <ethash/keccak.h>
#include <string.h>

/* ATTRIBUTES */

/** inline */
#if _MSC_VER || __STDC_VERSION__
#define INLINE inline
#else
#define INLINE
#endif

/** [[always_inline]] */
#if _MSC_VER
#define ALWAYS_INLINE __forceinline
#elif defined(__has_attribute) && __STDC_VERSION__
#if __has_attribute(always_inline)
#define ALWAYS_INLINE __attribute__((always_inline))
#endif
#endif
#if !defined(ALWAYS_INLINE)
#define ALWAYS_INLINE
#endif

/** [[no_sanitize()]] */
#if __clang__
#define NO_SANITIZE(sanitizer) \
    __attribute__((no_sanitize(sanitizer)))
#else
#define NO_SANITIZE(sanitizer)
#endif

/* END OF ATTRIBUTES */

#if _WIN32
/* On Windows assume little endian. */
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN
#elif __APPLE__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define to_le64(X) X
#else
#define to_le64(X) __builtin_bswap64(X)
#endif


/** Loads 64-bit integer from given memory location as little-endian number. */
static INLINE ALWAYS_INLINE uint64_t load_le(const uint8_t* data)
{
    /* memcpy is the best way of expressing the intention. Every compiler will
       optimize is to single load instruction if the target architecture
       supports unaligned memory access (GCC and clang even in O0).
       This is great trick because we are violating C/C++ memory alignment
       restrictions with no performance penalty. */
    uint64_t word;
    memcpy(&word, data, sizeof(word));
    return to_le64(word);
}

// This is for 256 bits **only**
static INLINE ALWAYS_INLINE void keccakUint8_256(
    uint8_t* out, const uint8_t* data, size_t size)
{
    static const size_t word_size = sizeof(uint64_t);
    const size_t hash_size = 256 / 8;
    const size_t block_size = (1600 - 256 * 2) / 8;

    size_t i;
    uint64_t* state_iter;
    uint64_t last_word = 0;
    uint8_t* last_word_iter = (uint8_t*)&last_word;

    // We don't use out here because state has to be a *25* uint64_t array
    uint64_t state[25] = {0};

    while (size >= block_size)
    {
        for (i = 0; i < (block_size / word_size); ++i)
        {
            state[i] ^= load_le(data);
            data += word_size;
        }

        ethash_keccakf1600(state);

        size -= block_size;
    }

    state_iter = state;

    while (size >= word_size)
    {
        *state_iter ^= load_le(data);
        ++state_iter;
        data += word_size;
        size -= word_size;
    }

    while (size > 0)
    {
        *last_word_iter = *data;
        ++last_word_iter;
        ++data;
        --size;
    }
    *last_word_iter = 0x01;
    *state_iter ^= to_le64(last_word);

    state[(block_size / word_size) - 1] ^= 0x8000000000000000;

    ethash_keccakf1600(state);
    
    for (int i,x = 0; i < (hash_size / word_size); ++i, x+= 8) {
        // Instead of copying to a uint64_t* out like the old Ethereum library. we cast the state directly to uint8_t* out.
        // This makes mandatory the usage of a wrapper that enforces the string used as a parameter to be 32 bytes in size and this function to be 256 bits.
        // Be aware of to_le64 being a macro and not a function.
        out[x] = to_le64(state[i]);
        out[x+1] = to_le64(state[i]) >> 8;
        out[x+2] = to_le64(state[i]) >> 16;
        out[x+3] = to_le64(state[i]) >> 24;
        out[x+4] = to_le64(state[i]) >> 32;
        out[x+5] = to_le64(state[i]) >> 40;
        out[x+6] = to_le64(state[i]) >> 48;
        out[x+7] = to_le64(state[i]) >> 56;
    }
}


#endif // KECCAK_HPP