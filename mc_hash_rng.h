// mc_hash_rng.h - v1.0 - public domain, initial release 2021-09-15 - Miguel A. Friginal
//
// A hash-based pseudo-random number generator.
//
// A single-header-file library that provides a stateless random number generator using 
// hash functions, offering very good distribution, good speed, determinism (identical
// inputs always produce identical outputs), and random access (the sequence of numbers
// produced by a seed, can be accessed directly through an index without generating all
// previous values). All functions are pure functions. Utility functions are provided for
// different return values commonly used in procedural generation. The range functions
// also provide unbiased results. The library passes PractRand v0.96 expanded test set
// (-te 1), extra folding (-tf 2), to 1TB.
//
//
// This library is based on the work of many online sources, mainly:
//
//   Math for Game Programmers: Noise-Based RNG by Squirrel Eiserloh
//      https://youtu.be/LWFzPP8ZbdU
//
//   A Primer on Repeatable Random Numbers by Rune Skovbo Johansen
//      https://blog.unity.com/technology/a-primer-on-repeatable-random-numbers
//
//   Don't Generate, Hash! by Andrew Clifton
//      https://youtu.be/e4b--cyXEsM
//      https://twicetwo.com/files/generate-hash-notes.md
//
//   Efficiently Generating a Number in a Range by Melissa E. O'Neill
//      https://www.pcg-random.org/posts/bounded-rands.html
//
//   Tricks With the Floating-Point Format by Bruce Dawson
//      https://randomascii.wordpress.com/2012/01/11/tricks-with-the-floating-point-format/
//
//   xxHash
//      https://cyan4973.github.io/xxHash/
//      https://github.com/Cyan4973/xxHash
//
//   Fast Splittable Pseudorandom Number Generators by Steele, Lea & Flood (splitmix64)
//      https://doi.org/10.1145/2660193.2660195
//
//   Better Bit Mixing by David Stafford (mix13)
//      https://zimbry.blogspot.com/2011/09/better-bit-mixing-improving-on.html
//
//   MUM hash by Vladimir Makarov / wyhash by Wang Yi (the multiply-and-fold primitive)
//      https://github.com/vnmakarov/mum-hash
//      https://github.com/wangyi-fudan/wyhash
//
//   Sean Barrett's single-file public domain libraries in C/C++
//      https://github.com/nothings/stb
//
//   Lessons learned about how to make a header-file library by Sean Barrett
//      https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
//   PractRand by Chris Doty-Humphrey
//      https://pracrand.sourceforge.net
//
//   How to Test with PractRand by Melissa E. O'Neill
//      https://www.pcg-random.org/posts/how-to-test-with-practrand.html
//
//
// History:
//
//      1.0 (2026-05-31) Output-changing rework (every emitted value differs from v0.9):
//         - Per-word absorb changed from a full splitmix64 round to a single
//           data-dependent 32x32->64 multiply (word XORed into the low lane, times the
//           odd-forced high lane) plus a golden-ratio Weyl add. This is ~half the
//           multiplies per word of v0.9, which restores multi-dimensional hashing speed.
//         - Output is still finalized by a full splitmix64 round, so the final avalanche
//           matches v0.9; only the absorb is cheaper.
//         - 64-bit state and high-lane seed placement are unchanged from v0.9.
//
//      0.9 (2026-05-30) Output-changing rework (every emitted value differs from v0.8):
//         - Hash core replaced with a splitmix64-based mixer: a 64-bit state absorbs the
//           input one 32-bit word at a time (a full splitmix64 round per word) and is
//           truncated to its top 32 bits. The seed is placed in the high 32 bits of the
//           state so it is mixed strongly and cannot collide with coordinate data.
//         - Replaces xxHash (XXH32), whose small-input path under-mixed and failed
//           PractRand (FPF/Gap) within ~1 GB on counter-like streams. splitmix64's mixer
//           is validated as a counter generator, which suits the small, near-sequential
//           inputs this library hashes.
//
//      0.8 (2026-05-29) Capability and documentation additions (no value changes):
//         - The main hash now accepts any length: XXH32's 1-3 byte tail step was added,
//           so callers no longer need to pad data to a multiple of 4. Multiple-of-4
//           inputs never enter the tail loop, so all v0.7 outputs are unchanged; the 
//           length assert was removed.
//         - Documented the struct-padding hazard for by-value struct hashing.
//         - Documented determinism/portability: integer inputs are endian-independent,
//           the float helpers are exact on any IEEE-754 binary32 platform, raw byte
//           buffers are endian-dependent.
//
//      0.7 (2026-05-29) Output-changing rework (every emitted value differs from v0.6):
//         - Hash replaced with xxHash (XXH32, by Yann Collet), applied over native 32-bit
//           words via aliasing-safe memcpy; much stronger avalanche than the previous
//           Squirrel3-style hash.
//         - under_limit range reduction replaced with Lemire's multiply-shift method
//           (64-bit intermediate), dropping the bitmask rejection loop and the clz
//           dependency, with far less rejection. An upper_bound of 0 now denotes the full
//           2^32 range.
//         - Added MCHR_UINT64 (uint64_t / unsigned long long) for the multiply; reusable
//           by future wider-precision helpers.
//
//      0.6 (2026-05-29) Bug fixes and cleanup:
//         - neg_one_to_one now returns a properly closed [-1,1], derived from zero_to_one
//           (2*z - 1): both endpoints reachable, exactly equidistant, and computed
//           entirely in float.
//         - chance now draws from a half-open [0,1) value, so a probability of 1.0 is
//           always true and 0.0 always false (rate is unbiased).
//         - int_in_range computes its span in unsigned arithmetic to avoid
//           signed-overflow UB on ranges wider than INT_MAX.
//         - Float helpers simplified ((float)num / max_range); removed a dead mask and
//           the pass-through temporaries.
//         - Documentation fixes: float spacing (1/2^24 and 1/2^23), the usage example,
//           and 'HEADER' / 'hash function' typos.
//
//      0.5 (2021-09-15) First released version.
//
//
// Compiling:
//
//   In one C/C++ file that #includes this file, do
//
//      #define MCHR_IMPLEMENTATION
//      #include "mc_hash_rng.h"
//
//   Optionally, #define MCHR_STATIC before including the header to cause definitions
//   to be private to the implementation file (i.e. to be "static" instead of "extern";
//   -Wall will warn about unused functions in this case, -Wno-unused-function can help),
//   and #define MCHR_USE_STDINT to have integer parameters and return values be
//   "uint32_t" and "int32_t" instead of "unsigned int" and "int".
//
//
// License:
//
//   See end of file for license information.
//
//
// Usage:
//
//   All functions require passing an unsigned int seed, and some data to be hashed. The
//   data can be passed as a pointer to any block of memory and its size in bytes (any
//   length and alignment work):
//
//         float data[] = { ... };
//         unsigned int seed = 0;
//         unsigned int random_value = mchr_get_hash_uint(data, sizeof(data), seed);
//
//   or directly as up to 4 integer parameters through helper functions (look for 1d, 2d,
//   3d, or 4d as part of the function name):
//
//         int data1 = 130; int data2 = 23;
//         unsigned int seed = 0;
//         unsigned int random_value = mchr_get_2d_hash_uint(data1, data2, seed);
//
//   Use the range functions to obtain numbers without modulo bias, e.g. don't do this:
//
//         int zero_to_nine_with_bias = mchr_get_1d_hash_uint(data1, seed) % 10;
//
//   do this instead:
//
//         int zero_to_nine_no_bias = mchr_get_1d_hash_int_in_range(data1, seed, 0, 9);
//
//   Notice most range functions use closed ranges (including both minimum and maximum).
//
//   Apart from functions returning integer types, there are helper functions that return
//   floats between 0 and 1 or between -1 and 1 (inclusive, closed ranges). The 
//   zero-to-one functions return equidistant floats 1.0 / 2^24 apart; the neg-one-to-one 
//   functions span twice the width, so they return equidistant floats 1.0 / 2^23 apart. 
//   The even spacing matters since normally there are more representable floats between 
//   0.0 and 0.5 than between 0.5 and 1.0; these helpers avoid that imbalance.
//
//         float zero_to_one = mchr_get_3d_hash_zero_to_one(data1, data2, data3, seed);
//
//   There are also helper functions that return true if a hash value generated between
//   0.0 and 1.0 is less than a certain probability:
//
//         if (mchr_get_1d_chance(data1, seed, 0.5)) // 50% probability
//
//
// More about seeds and data indices/positions:
//
//   Both the seed and at least one data index are necessary for the hash function to
//   return a value. In practice this can be used...
//
//      1) To make random calls independent of the calling order. Where with a standard
//         random function you would do:
//
//               srand(seed);
//               int num0 = rand() % limit;
//               int num1 = rand() % limit;
//               int num2 = rand() % limit;
//               ...
//               int num51 = rand() % limit;
//
//         With this library you could do:
//
//               int num0 = mchr_get_1d_hash_int_in_range(0, seed, 0, limit - 1);
//               int num51 = mchr_get_1d_hash_int_in_range(51, seed, 0, limit - 1);
//               int num1 = mchr_get_1d_hash_int_in_range(1, seed, 0, limit - 1);
//
//         Making the seed and position explicit in the call makes it independent of the
//         order in which the function is called.
//
//      2) To use any kind of data as part of the repeatable seed for the pseudo-random
//         number generator. E.g.:
//
//               typedef struct tree_data_t {
//                   float position[3];
//                   enum tree_type type;
//               } tree_data_t;
// 
//               tree_data_t data = {
//                   .position = {-10.5, -0.16, 32.5},
//                   .type = K_TREE_TYPE_OAK
//               };
//               const unsigned int seed_for_tree_heights = 234234;
// 
//               float tree_height = mchr_get_hash_zero_to_one(&data, sizeof(tree_data_t),
//                                                             seed_for_tree_heights);
//
//         Caution: hashing a struct by value (&data, sizeof(data)) also hashes any
//         padding bytes the compiler inserts between or after members, and padding is
//         indeterminate - so two structs that are equal field-by-field can hash
//         differently, and the same struct can hash differently across compilers/ABIs. To
//         hash a struct reproducibly, either ensure it has no padding (and
//         zero-initialize it) or hash the individual fields (e.g. via the 2d/3d/4d
//         helpers) instead.
//
//
// Determinism and portability:
//
//   Identical inputs always produce identical outputs, and any value can be addressed
//   directly from its seed and index (random access). The integer index helpers (1d-4d,
//   or arrays of MCHR_INT / MCHR_UINT) return the same result on big- and little-endian
//   platforms, because whole 32-bit words are read in their native layout. The float
//   helpers (zero_to_one, neg_one_to_one) and chance are bit-identical on any IEEE-754
//   binary32 platform: their internal arithmetic is exact (it lands on representable
//   values), so no special floating-point settings are required.
//
//   For RAW byte buffers (as opposed to the integer helpers), cross-endian results
//   differ: whole words are read natively, while a trailing 1-3 byte remainder is read
//   one byte at a time in memory order. If you need a raw byte buffer to hash identically
//   across endianness, feed the data through the integer helpers or normalize its byte
//   order yourself.
//
//
// Thread-safety:
//
//   Although all functions are pure functions (depending only on their input parameters),
//   you will need to lock access to the index buffers pointed at by the random length
//   hash functions like "mchr_get_hash_uint()" or "mchr_get_chance()".

#ifndef MCHR_INCLUDE_MC_HASH_RNG_H
#define MCHR_INCLUDE_MC_HASH_RNG_H

// std includes here
#include <stdlib.h>
#include <stdbool.h>

#ifdef MCHR_STATIC
#define MCHR_DEF static
#else
#ifdef __cplusplus
#define MCHR_DEF extern "C"
#else
#define MCHR_DEF extern
#endif
#endif

#ifdef MCHR_USE_STDINT
#include <stdint.h>
#define MCHR_UINT uint32_t
#define MCHR_INT int32_t
#define MCHR_UINT64 uint64_t
#else
#define MCHR_UINT unsigned int
#define MCHR_INT int
#define MCHR_UINT64 unsigned long long
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------------------
// Raw pseudorandom hash functions (random-access / deterministic) returning an unsigned
//  integer. Basis of all other functions.
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_UINT mchr_get_hash_uint( const void* index_buffer, size_t len, MCHR_UINT seed );
MCHR_DEF MCHR_UINT mchr_get_1d_hash_uint( MCHR_INT pos, MCHR_UINT seed );
MCHR_DEF MCHR_UINT mchr_get_2d_hash_uint( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed );
MCHR_DEF MCHR_UINT mchr_get_3d_hash_uint( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed );
MCHR_DEF MCHR_UINT mchr_get_4d_hash_uint( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed );

// ---------------------------------------------------------------------------------------
// A version of `mchr_get_hash_uint()` that returns unbiased unsigned integers in the
//  left-closed interval (including min but not max) between zero and an upper limit. Used
//  internally by all other range functions.
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_UINT mchr_get_hash_uint_under_limit(const void* index_buffer, size_t len, MCHR_UINT seed, MCHR_UINT upper_bound);

// ---------------------------------------------------------------------------------------
// Same functions, returning an unsigned integer in the specified closed (including both
//  min and max) range.
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_UINT mchr_get_hash_uint_in_range( const void* index_buffer, size_t len, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max );
MCHR_DEF MCHR_UINT mchr_get_1d_hash_uint_in_range( MCHR_INT pos, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max );
MCHR_DEF MCHR_UINT mchr_get_2d_hash_uint_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max );
MCHR_DEF MCHR_UINT mchr_get_3d_hash_uint_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max );
MCHR_DEF MCHR_UINT mchr_get_4d_hash_uint_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max );

// ---------------------------------------------------------------------------------------
// Same functions, returning an integer in the specified closed (including both min and
//  max) range.
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_INT mchr_get_hash_int_in_range( const void* index_buffer, size_t len, MCHR_UINT seed, MCHR_INT min, MCHR_INT max );
MCHR_DEF MCHR_INT mchr_get_1d_hash_int_in_range( MCHR_INT pos, MCHR_UINT seed, MCHR_INT min, MCHR_INT max );
MCHR_DEF MCHR_INT mchr_get_2d_hash_int_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed, MCHR_INT min, MCHR_INT max );
MCHR_DEF MCHR_INT mchr_get_3d_hash_int_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed, MCHR_INT min, MCHR_INT max );
MCHR_DEF MCHR_INT mchr_get_4d_hash_int_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed, MCHR_INT min, MCHR_INT max );

// ---------------------------------------------------------------------------------------
// Same functions, mapped to equidistant floats in the closed [0,1] range.
// ---------------------------------------------------------------------------------------
MCHR_DEF float mchr_get_hash_zero_to_one( const void* index_buffer, size_t len, MCHR_UINT seed );
MCHR_DEF float mchr_get_1d_hash_zero_to_one( MCHR_INT pos, MCHR_UINT seed );
MCHR_DEF float mchr_get_2d_hash_zero_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed );
MCHR_DEF float mchr_get_3d_hash_zero_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed );
MCHR_DEF float mchr_get_4d_hash_zero_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed );

// ---------------------------------------------------------------------------------------
// Same functions, mapped to equidistant floats in the closed [-1,1] range.
// ---------------------------------------------------------------------------------------
MCHR_DEF float mchr_get_hash_neg_one_to_one( const void* index_buffer, size_t len, MCHR_UINT seed );
MCHR_DEF float mchr_get_1d_hash_neg_one_to_one( MCHR_INT pos, MCHR_UINT seed );
MCHR_DEF float mchr_get_2d_hash_neg_one_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed );
MCHR_DEF float mchr_get_3d_hash_neg_one_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed );
MCHR_DEF float mchr_get_4d_hash_neg_one_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed );

// ---------------------------------------------------------------------------------------
// Same functions, returning true if the left-closed [0,1) range result is under
//  probability_of_true.
// ---------------------------------------------------------------------------------------
MCHR_DEF bool mchr_get_chance( const void* index_buffer, size_t len, MCHR_UINT seed, float probability_of_true );
MCHR_DEF bool mchr_get_1d_chance( MCHR_INT pos, MCHR_UINT seed, float probability_of_true );
MCHR_DEF bool mchr_get_2d_chance( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed, float probability_of_true );
MCHR_DEF bool mchr_get_3d_chance( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed, float probability_of_true );
MCHR_DEF bool mchr_get_4d_chance( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed, float probability_of_true );

#ifdef __cplusplus
}
#endif

// END OF HEADER FILE --------------------------------------------------------------------
#endif // MCHR_INCLUDE_MC_HASH_RNG_H


#ifdef MCHR_IMPLEMENTATION

// std includes here
#include <assert.h>
#include <string.h>

// splitmix64 finalizer (Steele, Lea & Flood, "Fast Splittable Pseudorandom Number
//  Generators", 2014; constants from David Stafford's mix13). Used here as the output
//  finalizer: it gives the result a full, BigCrush-strength avalanche regardless of how
//  cheaply the input was absorbed.
static MCHR_UINT64 mchr_priv_smix64(MCHR_UINT64 z) {
    const MCHR_UINT64 MCHR_SMIX_C1 = 0xBF58476D1CE4E5B9ULL;
    const MCHR_UINT64 MCHR_SMIX_C2 = 0x94D049BB133111EBULL;

    z = (z ^ (z >> 30)) * MCHR_SMIX_C1;
    z = (z ^ (z >> 27)) * MCHR_SMIX_C2;
    return z ^ (z >> 31);
}

// ---------------------------------------------------------------------------------------
// Main hash. A splitmix64 finalizer over a multiply-plus-Weyl absorb. 
//
// - splitmix64 (Stafford mix13) for the finalizer.
// - A data-dependent 32×32→64 lane multiply (the MUM/wyhash multiply primitive, minus the
//   fold) for the absorb.
// - A golden-ratio Weyl step for the schedule.
// - Seed in the high lane, top 32 bits out.
//
// Two load-bearing details: the seed sits in the HIGH lane so seed=A,index=B can't
//  collide with seed=B,index=A; and the 64->32 truncation keeps the map many-to-one.
//
// Whole words are read via memcpy (alias-safe, unaligned-safe); a 1-3 byte tail is folded
//  in last, so multiple-of-4 inputs never hit that branch. Integer helpers are
//  endian-independent; raw byte buffers are not (see the determinism note).
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_UINT mchr_get_hash_uint(const void* index_buffer, size_t len, MCHR_UINT seed) {
    const unsigned char* p = (const unsigned char*)index_buffer;
    const unsigned char* const end = p + len;
    MCHR_UINT w;

    // Seed in the high lane; the golden-ratio increment moves a zero seed off the origin.
    MCHR_UINT64 h = ((MCHR_UINT64)seed << 32) + 0x9E3779B97F4A7C15ULL;

    // Whole 32-bit words: one data-dependent multiply each, then a Weyl advance (the add
    //  also nudges the state off zero on the rare zero product).
    while (p + 4 <= end) {
        memcpy(&w, p, 4);
        h = (MCHR_UINT64)((MCHR_UINT)h ^ w) * ((MCHR_UINT)(h >> 32) | 1u);
        h += 0x9E3779B97F4A7C15ULL;
        p += 4;
    }

    // Trailing 1-3 bytes (taken individually, in memory order). Never runs for
    //  multiple-of-4 lengths, so the integer index helpers never reach this branch.
    if (p < end) {
        MCHR_UINT tail = 0;
        int shift = 0;
        while (p < end) {
            tail |= (MCHR_UINT)(*p) << shift;
            shift += 8;
            ++p;
        }
        h = (MCHR_UINT64)((MCHR_UINT)h ^ tail) * ((MCHR_UINT)(h >> 32) | 1u);
        h += 0x9E3779B97F4A7C15ULL;
    }

    // Fold in the length, finalize with a full splitmix64 round, keep the top 32 bits.
    h = mchr_priv_smix64(h + (MCHR_UINT64)len);
    return (MCHR_UINT)(h >> 32);
}

// ---------------------------------------------------------------------------------------
// Return a uniformly distributed value in the left-closed interval [0, upper_bound) with
//  no "modulo bias", using Lemire's multiply-and-shift method (see Melissa O'Neill,
//  https://www.pcg-random.org/posts/bounded-rands.html). The 32-bit hash is multiplied by
//  upper_bound into a 64-bit product; the high 32 bits give the result, and the rejection
//  branch (which computes a single modulo, and only when needed) is taken rarely. By
//  convention an upper_bound of 0 means the full 2^32 range, returning the raw hash.
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_UINT mchr_get_hash_uint_under_limit(const void* index_buffer, size_t len, MCHR_UINT seed, MCHR_UINT upper_bound) {
    if (upper_bound == 0)
        return mchr_get_hash_uint(index_buffer, len, seed);

    MCHR_UINT64 product = (MCHR_UINT64)mchr_get_hash_uint(index_buffer, len, seed) * upper_bound;
    MCHR_UINT low = (MCHR_UINT)product;
    if (low < upper_bound) {
        MCHR_UINT threshold = (0u - upper_bound) % upper_bound;   // == 2^32 mod upper_bound
        while (low < threshold) {
            seed += 1;
            product = (MCHR_UINT64)mchr_get_hash_uint(index_buffer, len, seed) * upper_bound;
            low = (MCHR_UINT)product;
        }
    }
    return (MCHR_UINT)(product >> 32);
}

// ---------------------------------------------------------------------------------------
// Private function to convert an unsigned integer in [0, 2^24] into a float in the closed
//  [0,1] range, evenly distributed: equally many outputs land in [0.0, 0.5] as in
//  [0.5, 1.0] (a raw float grid is denser toward 0, so that even split is not automatic).
// ---------------------------------------------------------------------------------------
static float mchr_priv_uint_to_zero_one(MCHR_UINT num) {
    const MCHR_UINT max_range = 1 << 24;
    assert(num <= max_range);
    if (num >= max_range)
        return 1.0f;
    return (float)num / max_range;
}

// ---------------------------------------------------------------------------------------
// Private function to convert an unsigned integer in the range [0, 2^24) into a float in
//  the left-closed [0,1) range (1.0 excluded). Used by the chance functions, whose
//  "less than probability" test needs 1.0 to be unreachable so that a probability of 1.0
//  is always true and a probability of 0.0 is always false (and the true-probability 
//  stays unbiased).
// ---------------------------------------------------------------------------------------
static float mchr_priv_uint_to_zero_one_excl(MCHR_UINT num) {
    const MCHR_UINT max_range = 1 << 24;
    assert(num < max_range);
    num &= (max_range - 1);
    return (float)num / max_range;
}

// ---------------------------------------------------------------------------------------
// Unsigned integer result.
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_UINT mchr_get_1d_hash_uint( MCHR_INT pos, MCHR_UINT seed ) {
    return mchr_get_hash_uint(&pos, sizeof(MCHR_INT), seed);
}

MCHR_DEF MCHR_UINT mchr_get_2d_hash_uint( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY };
    return mchr_get_hash_uint(array, sizeof(array), seed);
}

MCHR_DEF MCHR_UINT mchr_get_3d_hash_uint( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY, posZ };
    return mchr_get_hash_uint(array, sizeof(array), seed);
}


MCHR_DEF MCHR_UINT mchr_get_4d_hash_uint( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY, posZ, posT };
    return mchr_get_hash_uint(array, sizeof(array), seed);
}

// ---------------------------------------------------------------------------------------
// Unsigned integer in range (closed range, including both limits).
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_UINT mchr_get_hash_uint_in_range( const void* index_buffer, size_t len, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max ) {
    assert(min <= max);
    return mchr_get_hash_uint_under_limit(index_buffer, len, seed, max - min + 1) + min;
}

MCHR_DEF MCHR_UINT mchr_get_1d_hash_uint_in_range( MCHR_INT pos, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max ) {
    assert(min <= max);
    return mchr_get_hash_uint_under_limit(&pos, sizeof(MCHR_INT), seed, max - min + 1) + min;
}

MCHR_DEF MCHR_UINT mchr_get_2d_hash_uint_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max ) {
    assert(min <= max);
    MCHR_INT array[] = { posX, posY };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, max - min + 1) + min;
}

MCHR_DEF MCHR_UINT mchr_get_3d_hash_uint_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max ) {
    assert(min <= max);
    MCHR_INT array[] = { posX, posY, posZ };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, max - min + 1) + min;
}

MCHR_DEF MCHR_UINT mchr_get_4d_hash_uint_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max ) {
    assert(min <= max);
    MCHR_INT array[] = { posX, posY, posZ, posT };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, max - min + 1) + min;
}

// ---------------------------------------------------------------------------------------
// Integer in range (closed range, including both limits). The span is computed in
//  unsigned arithmetic ((MCHR_UINT)max - (MCHR_UINT)min) so a range wider than INT_MAX
//  does not overflow the signed subtraction.
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_INT mchr_get_hash_int_in_range( const void* index_buffer, size_t len, MCHR_UINT seed, MCHR_INT min, MCHR_INT max ) {
    assert(min <= max);
    return mchr_get_hash_uint_under_limit(index_buffer, len, seed, (MCHR_UINT)max - (MCHR_UINT)min + 1) + min;
}

MCHR_DEF MCHR_INT mchr_get_1d_hash_int_in_range( MCHR_INT pos, MCHR_UINT seed, MCHR_INT min, MCHR_INT max ) {
    assert(min <= max);
    return mchr_get_hash_uint_under_limit(&pos, sizeof(MCHR_INT), seed, (MCHR_UINT)max - (MCHR_UINT)min + 1) + min;
}

MCHR_DEF MCHR_INT mchr_get_2d_hash_int_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed, MCHR_INT min, MCHR_INT max ) {
    assert(min <= max);
    MCHR_INT array[] = { posX, posY };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (MCHR_UINT)max - (MCHR_UINT)min + 1) + min;
}

MCHR_DEF MCHR_INT mchr_get_3d_hash_int_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed, MCHR_INT min, MCHR_INT max ) {
    assert(min <= max);
    MCHR_INT array[] = { posX, posY, posZ };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (MCHR_UINT)max - (MCHR_UINT)min + 1) + min;
}

MCHR_DEF MCHR_INT mchr_get_4d_hash_int_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed, MCHR_INT min, MCHR_INT max ) {
    assert(min <= max);
    MCHR_INT array[] = { posX, posY, posZ, posT };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (MCHR_UINT)max - (MCHR_UINT)min + 1) + min;
}

// ---------------------------------------------------------------------------------------
// Float from 0.0 to 1.0 (closed range, including both limits).
// ---------------------------------------------------------------------------------------
MCHR_DEF float mchr_get_hash_zero_to_one( const void* index_buffer, size_t len, MCHR_UINT seed ) {
    return mchr_priv_uint_to_zero_one(
        mchr_get_hash_uint_under_limit(index_buffer, len, seed, (1 << 24) + 1)
    );
}

MCHR_DEF float mchr_get_1d_hash_zero_to_one( MCHR_INT pos, MCHR_UINT seed ) {
    return mchr_priv_uint_to_zero_one(
        mchr_get_hash_uint_under_limit(&pos, sizeof(MCHR_INT), seed, (1 << 24) + 1)
    );
}

MCHR_DEF float mchr_get_2d_hash_zero_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY };
    return mchr_priv_uint_to_zero_one(
        mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (1 << 24) + 1)
    );
}

MCHR_DEF float mchr_get_3d_hash_zero_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY, posZ };
    return mchr_priv_uint_to_zero_one(
        mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (1 << 24) + 1)
    );
}

MCHR_DEF float mchr_get_4d_hash_zero_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY, posZ, posT };
    return mchr_priv_uint_to_zero_one(
        mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (1 << 24) + 1)
    );
}

// ---------------------------------------------------------------------------------------
// Return true if a left-closed [0,1) range random result is under probability_of_true.
// The [0,1) source (1.0 excluded) is deliberate: it makes a probability of 1.0 always
//  true and a probability of 0.0 always false, and keeps the true-probability unbiased.
// ---------------------------------------------------------------------------------------
MCHR_DEF bool mchr_get_chance( const void* index_buffer, size_t len, MCHR_UINT seed, float probability_of_true ) {
    assert((0.0 <= probability_of_true) && (probability_of_true <= 1.0));
    MCHR_UINT result = mchr_get_hash_uint_under_limit(index_buffer, len, seed, 1 << 24);
    return mchr_priv_uint_to_zero_one_excl(result) < probability_of_true;
}

MCHR_DEF bool mchr_get_1d_chance( MCHR_INT pos, MCHR_UINT seed, float probability_of_true ) {
    assert((0.0 <= probability_of_true) && (probability_of_true <= 1.0));
    MCHR_UINT result = mchr_get_hash_uint_under_limit(&pos, sizeof(MCHR_INT), seed, 1 << 24);
    return mchr_priv_uint_to_zero_one_excl(result) < probability_of_true;
}

MCHR_DEF bool mchr_get_2d_chance( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed, float probability_of_true ) {
    assert((0.0 <= probability_of_true) && (probability_of_true <= 1.0));
    MCHR_INT array[] = { posX, posY };
    MCHR_UINT result = mchr_get_hash_uint_under_limit(array, sizeof(array), seed, 1 << 24);
    return mchr_priv_uint_to_zero_one_excl(result) < probability_of_true;
}

MCHR_DEF bool mchr_get_3d_chance( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed, float probability_of_true ) {
    assert((0.0 <= probability_of_true) && (probability_of_true <= 1.0));
    MCHR_INT array[] = { posX, posY, posZ };
    MCHR_UINT result = mchr_get_hash_uint_under_limit(array, sizeof(array), seed, 1 << 24);
    return mchr_priv_uint_to_zero_one_excl(result) < probability_of_true;
}

MCHR_DEF bool mchr_get_4d_chance( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed, float probability_of_true ) {
    assert((0.0 <= probability_of_true) && (probability_of_true <= 1.0));
    MCHR_INT array[] = { posX, posY, posZ, posT };
    MCHR_UINT result = mchr_get_hash_uint_under_limit(array, sizeof(array), seed, 1 << 24);
    return mchr_priv_uint_to_zero_one_excl(result) < probability_of_true;
}

// ---------------------------------------------------------------------------------------
// Float from -1.0 to 1.0 (closed range, including both limits).
// Derived from the closed [0,1] generator: 2*z - 1 maps [0,1] onto [-1,1] with both
//  endpoints included. Because z lands on the exact 1/2^24 grid, this arithmetic is exact
//  (kept in float, no double round-trip), giving equidistant outputs 1/2^23 apart.
// ---------------------------------------------------------------------------------------
MCHR_DEF float mchr_get_hash_neg_one_to_one( const void* index_buffer, size_t len, MCHR_UINT seed ) {
    return 2.0f * mchr_get_hash_zero_to_one(index_buffer, len, seed) - 1.0f;
}

MCHR_DEF float mchr_get_1d_hash_neg_one_to_one( MCHR_INT pos, MCHR_UINT seed ) {
    return 2.0f * mchr_get_1d_hash_zero_to_one(pos, seed) - 1.0f;
}

MCHR_DEF float mchr_get_2d_hash_neg_one_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed ) {
    return 2.0f * mchr_get_2d_hash_zero_to_one(posX, posY, seed) - 1.0f;
}

MCHR_DEF float mchr_get_3d_hash_neg_one_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed ) {
    return 2.0f * mchr_get_3d_hash_zero_to_one(posX, posY, posZ, seed) - 1.0f;
}

MCHR_DEF float mchr_get_4d_hash_neg_one_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed ) {
    return 2.0f * mchr_get_4d_hash_zero_to_one(posX, posY, posZ, posT, seed) - 1.0f;
}

#endif // MCHR_IMPLEMENTATION

/*
------------------------------------------------------------------------------------------
This software is available under the MIT license.
------------------------------------------------------------------------------------------
MIT License

Copyright (c) 2021 Miguel A. Friginal

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------------
*/
