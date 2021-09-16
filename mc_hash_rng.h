// mc_hash_rng.h - v0.5 - public domain, initial release 2021-09-15 - Miguel A. Friginal
//
// A hash-based pseudo-random number generator.
//
// This is a single-header-file library that provides a stateless random number generator
// using hash functions, offering very good distribution, good speed, determinism
// (identical inputs always produce identical outputs), and random access (the sequence of
// numbers produced by a seed, can be accessed directly through an index without
// generating all previous values). All functions are pure functions. Utility functions
// are provided for different return values commonly used in procedural generation. The
// range functions also provide unbiased results.
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
//   Sean Barrett's single-file public domain libraries in C/C++
//      https://github.com/nothings/stb
//
//   Lessons learned about how to make a header-file library by Sean Barrett
//      https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
//
// History:
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
//   to be private to the implementation file (i.e. to be "static" instead of "extern"),
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
//   data can be passed as a pointer to a 32-bit aligned block of memory and its size:
//
//          float data[] = { ... };
//          unsigned int seed = 0;
//          unsigned int random_value = mchr_get_hash_uint(data, sizeof(data), seed);
//
//   or directly as up to 4 integer parameters through helper functions (look for 1d, 2d,
//   3d, or 4d as part of the function name):
//
//          int data1 = 130; int data2 = 23;
//          unsigned int seed = 0;
//          unsigned int random_value = mchr_get_2d_hash_uint(data1, data2, seed)
//
//   Use the range functions to obtain numbers without modulo bias, e.g. don't do this:
//
//          int zero_to_nine_with_bias = mchr_get_1d_hash_int(data1, seed) % 10;
//
//   do this instead:
//
//          int zero_to_nine_no_bias = mchr_get_1d_hash_int_in_range(data1, seed, 0, 9);
//
//   Notice most range functions use closed ranges (including both minimum and maximum).
//
//   Apart from functions returning integer types, there are helper functions that return
//   floats between 0 and 1 or between -1 and 1 (inclusive). Adjacent floats returned by
//   these functions will always be 1.0 / 2^23 apart, offering an even distribution
//   (usually there are more discrete values between 0 and 0.5 than between 0.5 to 1).
//
//          float zero_to_one = mchr_get_3d_hash_zero_to_one(data1, data2, data3, seed);
//
//   There are also helper functions that return true if a hash value generated between
//   0.0 and 1.0 is less than a certain probability:
//
//          if (mchr_get_1d_chance(data1, seed, 0.5)) // 50% probability
//
//
// More about seeds and data indices/positions:
//
//   Both the seed and at least one data index are necessary for the hash function to
//   return a value. In practice this can be used...
//
//     1) To make random calls independent of the calling order. Where with a standard
//        random function you would do:
//
//              srand(seed);
//              int num0 = rand() % limit;
//              int num1 = rand() % limit;
//              int num2 = rand() % limit;
//              ...
//              int num51 = rand() % limit;
//
//        with this library you could do:
//
//              int num0 = mchr_get_1d_hash_int_in_range(0, seed, 0, limit - 1);
//              int num51 = mchr_get_1d_hash_int_in_range(51, seed, 0, limit - 1);
//              int num1 = mchr_get_1d_hash_int_in_range(1, seed, 0, limit - 1);
//
//        Making the seed and position explicit in the call makes it independent of the
//        order in which the function is called.
//
//     2) To use any kind of data as part of the repeatable seed for the pseudo-random
//        number generator. E.g.:
//
//              typedef struct tree_data_t {
//                  float position[3];
//                  enum tree_type type;
//              } tree_data_t;
//
//              tree_data_t data = {
//                  .position = {-10.5, -0.16, 32.5},
//                  .type = K_TREE_TYPE_OAK
//              };
//              const unsigned int seed_for_tree_heights = 234234;
//
//              float tree_height = mchr_get_hash_zero_to_one(&data, sizeof(tree_data_t),
//                                                            seed_for_tree_heights);
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
#else
#define MCHR_UINT unsigned int
#define MCHR_INT int
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

// END OF HEDER FILE ---------------------------------------------------------------------
#endif // MCHR_INCLUDE_MC_HASH_RNG_H


#ifdef MCHR_IMPLEMENTATION

// std includes here
#include <limits.h>
#include <assert.h>

#if defined(_MSC_VER) && !defined(__clang__)

#include <intrin.h>

static MCHR_UINT __inline clz(MCHR_UINT value) {
    unsigned long leading_zero = 0;
    if (_BitScanReverse(&leading_zero, value))
        return 31 - leading_zero;
    // This is undefined, I better choose 32 than 0
    return 32;
}

#else

#define clz(x) __builtin_clz(x)

#endif

static const MCHR_UINT MCHR_PRIMES[] = { 1,
                                         0x9E3779B1U,   // 0b1001 1110 0011 0111 0111 1001 1011 0001
                                         0x85EBCA77U,   // 0b1000 0101 1110 1011 1100 1010 0111 0111
                                         0xC2B2AE3DU,   // 0b1100 0010 1011 0010 1010 1110 0011 1101
                                         0x27D4EB2FU,   // 0b0010 0111 1101 0100 1110 1011 0010 1111
                                         0x165667B1U }; // 0b0001 0110 0101 0110 0110 0111 1011 0001
static const MCHR_INT MCHR_PRIMES_LEN = 6; //sizeof(MCHR_PRIMES) / sizeof(MCHR_PRIMES[0]);
static const MCHR_UINT MCHR_BIT_NOISE1 = 0x68E31DA4U;   // 0b0110 1000 1110 0011 0001 1101 1010 0100
static const MCHR_UINT MCHR_BIT_NOISE2 = 0xB5297A4DU;   // 0b1011 0101 0010 1001 0111 1010 0100 1101
static const MCHR_UINT MCHR_BIT_NOISE3 = 0x1B56C4E9U;   // 0b0001 1011 0101 0110 1100 0100 1110 1001

// ---------------------------------------------------------------------------------------
// This is the main hash table implementation. Currently using a modified Squirrel3 hash
//  (by Squirrel Eiserloh, see https://www.youtube.com/watch?v=LWFzPP8ZbdU) that works on
//  any data length, and doesn't return the same value for a zero index at all lengths.
// index_buffer needs to point to a block of memory with a length that is a multiple of
//  4 bytes (sizeof(uint32_t)).
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_UINT mchr_get_hash_uint(const void* index_buffer, size_t len, MCHR_UINT seed) {
    assert(len % sizeof(MCHR_UINT) == 0);

    MCHR_UINT num = 0;

    MCHR_INT i = 0;
    MCHR_INT i_next;
    MCHR_UINT *ptr = (MCHR_UINT*)index_buffer;
    while (len > 0) {
        i_next = (i + 1) % MCHR_PRIMES_LEN;
        num += *ptr++ * MCHR_PRIMES[i] + MCHR_PRIMES[i_next];
        i = i_next;
        len -= sizeof(MCHR_UINT);
    }

    num *= MCHR_BIT_NOISE1;
    num += seed;
    num ^= (num >> 8);
    num += MCHR_BIT_NOISE2;
    num ^= (num << 8);
    num *= MCHR_BIT_NOISE3;
    num ^= (num >> 8);

    return num;
}

// ---------------------------------------------------------------------------------------
// Calculate a uniformly distributed random number less than upper_bound avoiding
//  "modulo bias".
// Uniformity is achieved by trying successive ranges of bits from the random value, each
//  large enough to hold the desired upper bound, until a range holding a value less than
//  the bound is found.
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_UINT mchr_get_hash_uint_under_limit(const void* index_buffer, size_t len, MCHR_UINT seed, MCHR_UINT upper_bound) {
    if (upper_bound < 2)
        return 0;

    // find smallest 2**n -1 >= upper_bound
    MCHR_INT zeros = clz(upper_bound);
    MCHR_INT bits = CHAR_BIT * sizeof(MCHR_UINT) - zeros;
    MCHR_UINT mask = 0xFFFFFFFFU >> zeros;

    do {
        MCHR_UINT value = mchr_get_hash_uint(index_buffer, len, seed);

        // If low 2**n-1 bits satisfy the requested condition, return result
        MCHR_UINT result = value & mask;
        if (result < upper_bound) {
            return result;
        }

        // otherwise consume remaining bits of randomness looking for a satisfactory result.
        MCHR_INT bits_left = zeros;
        while (bits_left >= bits) {
            value >>= bits;
            result = value & mask;
            if (result < upper_bound) {
                return result;
            }
            bits_left -= bits;
        }
        seed += 1;
    } while (1);
}

// ---------------------------------------------------------------------------------------
// Private function to convert an unsigned integer in the range 0 to 2^24 into a float
//  from zero to 1 that is evenly distributed (with as many numbers in the interval from
//  0.0 to 0.5 than in the interval from 0.5 to 1.0; normally 0.0 to 0.5 has float the
//  precision than 0.5 to 1.0).
// ---------------------------------------------------------------------------------------
static float mchr_priv_uint_to_zero_one(MCHR_UINT num) {
    const MCHR_UINT max_range = 1 << 24;
    assert(num <= max_range);
    if (num >= max_range)
        return 1.0;
    num &= (max_range - 1);
    float result = num * 1.0f / max_range;
    return result;
}

// ---------------------------------------------------------------------------------------
// Private function to convert an unsigned integer in the range 0 to 2^25-1 into a float
//  from -1.0 to 1.0 that is evenly distributed (with as many numbers in the interval from
//  0.0 to 0.5 than in the interval from 0.5 to 1.0, both in positive and negative
//  directions).
// ---------------------------------------------------------------------------------------
static float mchr_priv_uint_to_neg_one_one(MCHR_UINT num) {
    const MCHR_UINT max_range = 1 << 25;
    assert(num < max_range);
    num &= (max_range - 1);
    float result = num * 1.0f / max_range;
    return result * 2.0 - 1.0;
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
    assert(min < max);
    return mchr_get_hash_uint_under_limit(index_buffer, len, seed, max - min + 1) + min;
}

MCHR_DEF MCHR_UINT mchr_get_1d_hash_uint_in_range( MCHR_INT pos, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max ) {
    assert(min < max);
    return mchr_get_hash_uint_under_limit(&pos, sizeof(MCHR_INT), seed, max - min + 1) + min;
}

MCHR_DEF MCHR_UINT mchr_get_2d_hash_uint_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max ) {
    assert(min < max);
    MCHR_INT array[] = { posX, posY };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, max - min + 1) + min;
}

MCHR_DEF MCHR_UINT mchr_get_3d_hash_uint_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max ) {
    assert(min < max);
    MCHR_INT array[] = { posX, posY, posZ };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, max - min + 1) + min;
}

MCHR_DEF MCHR_UINT mchr_get_4d_hash_uint_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed, MCHR_UINT min, MCHR_UINT max ) {
    assert(min < max);
    MCHR_INT array[] = { posX, posY, posZ, posT };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, max - min + 1) + min;
}

// ---------------------------------------------------------------------------------------
// Integer in range (closed range, including both limits).
// ---------------------------------------------------------------------------------------
MCHR_DEF MCHR_INT mchr_get_hash_int_in_range( const void* index_buffer, size_t len, MCHR_UINT seed, MCHR_INT min, MCHR_INT max ) {
    assert(min < max);
    return mchr_get_hash_uint_under_limit(index_buffer, len, seed, max - min + 1) + min;
}

MCHR_DEF MCHR_INT mchr_get_1d_hash_int_in_range( MCHR_INT pos, MCHR_UINT seed, MCHR_INT min, MCHR_INT max ) {
    assert(min < max);
    return mchr_get_hash_uint_under_limit(&pos, sizeof(MCHR_INT), seed, max - min + 1) + min;
}

MCHR_DEF MCHR_INT mchr_get_2d_hash_int_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed, MCHR_INT min, MCHR_INT max ) {
    assert(min < max);
    MCHR_INT array[] = { posX, posY };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, max - min + 1) + min;
}

MCHR_DEF MCHR_INT mchr_get_3d_hash_int_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed, MCHR_INT min, MCHR_INT max ) {
    assert(min < max);
    MCHR_INT array[] = { posX, posY, posZ };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, max - min + 1) + min;
}

MCHR_DEF MCHR_INT mchr_get_4d_hash_int_in_range( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed, MCHR_INT min, MCHR_INT max ) {
    assert(min < max);
    MCHR_INT array[] = { posX, posY, posZ, posT };
    return mchr_get_hash_uint_under_limit(array, sizeof(array), seed, max - min + 1) + min;
}

// ---------------------------------------------------------------------------------------
// Float from 0.0 to 1.0 (closed range, including both limits).
// ---------------------------------------------------------------------------------------
MCHR_DEF float mchr_get_hash_zero_to_one( const void* index_buffer, size_t len, MCHR_UINT seed ) {
    MCHR_UINT result = mchr_get_hash_uint_under_limit(index_buffer, len, seed, (1 << 24) + 1);
    return mchr_priv_uint_to_zero_one(result);
}

MCHR_DEF float mchr_get_1d_hash_zero_to_one( MCHR_INT pos, MCHR_UINT seed ) {
    MCHR_UINT result = mchr_get_hash_uint_under_limit(&pos, sizeof(MCHR_INT), seed, (1 << 24) + 1);
    return mchr_priv_uint_to_zero_one(result);
}

MCHR_DEF float mchr_get_2d_hash_zero_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY };
    MCHR_UINT result = mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (1 << 24) + 1);
    return mchr_priv_uint_to_zero_one(result);
}

MCHR_DEF float mchr_get_3d_hash_zero_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY, posZ };
    MCHR_UINT result = mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (1 << 24) + 1);
    return mchr_priv_uint_to_zero_one(result);
}

MCHR_DEF float mchr_get_4d_hash_zero_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY, posZ, posT };
    MCHR_UINT result = mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (1 << 24) + 1);
    return mchr_priv_uint_to_zero_one(result);
}

// ---------------------------------------------------------------------------------------
// Return true if a [0,1] range random result is under probability_of_true.
// ---------------------------------------------------------------------------------------
MCHR_DEF bool mchr_get_chance( const void* index_buffer, size_t len, MCHR_UINT seed, float probability_of_true ) {
    assert((0.0 <= probability_of_true) && (probability_of_true <= 1.0));
    return mchr_get_hash_zero_to_one(index_buffer, len, seed) < probability_of_true;
}

MCHR_DEF bool mchr_get_1d_chance( MCHR_INT pos, MCHR_UINT seed, float probability_of_true ) {
    assert((0.0 <= probability_of_true) && (probability_of_true <= 1.0));
    return mchr_get_1d_hash_zero_to_one(pos, seed) < probability_of_true;
}

MCHR_DEF bool mchr_get_2d_chance( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed, float probability_of_true ) {
    assert((0.0 <= probability_of_true) && (probability_of_true <= 1.0));
    return mchr_get_2d_hash_zero_to_one(posX, posY, seed) < probability_of_true;
}

MCHR_DEF bool mchr_get_3d_chance( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed, float probability_of_true ) {
    assert((0.0 <= probability_of_true) && (probability_of_true <= 1.0));
    return mchr_get_3d_hash_zero_to_one(posX, posY, posZ, seed) < probability_of_true;
}

MCHR_DEF bool mchr_get_4d_chance( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed, float probability_of_true ) {
    assert((0.0 <= probability_of_true) && (probability_of_true <= 1.0));
    return mchr_get_4d_hash_zero_to_one(posX, posY, posZ, posT, seed) < probability_of_true;
}

// ---------------------------------------------------------------------------------------
// Float from -1.0 to 1.0 (closed range, including both limits).
// ---------------------------------------------------------------------------------------
MCHR_DEF float mchr_get_hash_neg_one_to_one( const void* index_buffer, size_t len, MCHR_UINT seed ) {
    MCHR_UINT result = mchr_get_hash_uint_under_limit(index_buffer, len, seed, (1 << 25));
    return mchr_priv_uint_to_neg_one_one(result);
}

MCHR_DEF float mchr_get_1d_hash_neg_one_to_one( MCHR_INT pos, MCHR_UINT seed ) {
    MCHR_UINT result = mchr_get_hash_uint_under_limit(&pos, sizeof(MCHR_INT), seed, (1 << 25));
    return mchr_priv_uint_to_neg_one_one(result);
}

MCHR_DEF float mchr_get_2d_hash_neg_one_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY };
    MCHR_UINT result = mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (1 << 25));
    return mchr_priv_uint_to_neg_one_one(result);
}

MCHR_DEF float mchr_get_3d_hash_neg_one_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY, posZ };
    MCHR_UINT result = mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (1 << 25));
    return mchr_priv_uint_to_neg_one_one(result);
}

MCHR_DEF float mchr_get_4d_hash_neg_one_to_one( MCHR_INT posX, MCHR_INT posY, MCHR_INT posZ, MCHR_INT posT, MCHR_UINT seed ) {
    MCHR_INT array[] = { posX, posY, posZ, posT };
    MCHR_UINT result = mchr_get_hash_uint_under_limit(array, sizeof(array), seed, (1 << 25));
    return mchr_priv_uint_to_neg_one_one(result);
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