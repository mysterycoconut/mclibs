// mc_hash_practrand_test.c - feed mc_hash_rng output to PractRand's RNG_test via stdout.
//
// Build:
//   gcc -O3 -std=c99 -Wall -Wextra -I<dir-with-header> mc_hash_practrand_test.c -o mchr_gen
//
// Run (pipe into PractRand; tell it the words are 32-bit; 12345 used as seed):
//   ./mchr_gen ctr64 12345 | ./RNG_test stdin32
//
// What we test: the raw entropy source, mchr_get_hash_uint. The bounded/float/chance
// helpers are deterministic, already-proven-unbiased reductions of it (Lemire + the
// invariant suite), so PractRand belongs on the hash stream, not the wrappers.
//
// Modes (how the input is walked - this matters):
//   ctr64  (default) 64-bit counter fed through the 2d hash. Period 2^64, i.e. 
//                    effectively unbounded, so PractRand can run to TB scale with no
//                    period artifact.
//                    This is the primary test: a low-entropy incrementing input (lo bits
//                    change by 1 each step) is the hardest case for a hash-based RNG.
//   ctr32            32-bit counter fed through the 1d hash - the literal function most
//                    callers use. BUT the 32-bit index space repeats after 2^32 words
//                    (~16 GB of output), so a "failure" at/after 16 GB is the counter
//                    wrapping, not the hash. Only trust ctr32 results below ~16 GB.
//   seed             fixed index, incrementing seed. Tests the seed dimension. Same 2^32
//                    (~16 GB) period caveat as ctr32.
//
// The stream is native-endian 32-bit words; it runs until the reader closes the pipe.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#else
#include <signal.h>
#endif

#define MCHR_IMPLEMENTATION
#define MCHR_USE_STDINT
#include "mc_hash_rng.h"

#define BUFWORDS (1u << 16) // 256 KB per write; keeps PractRand, not us, the bottleneck

static MCHR_UINT g_buf[BUFWORDS];

int main(int argc, char** argv) {
    const char* mode = (argc > 1) ? argv[1] : "ctr64";
    MCHR_UINT   seed = (argc > 2) ? (MCHR_UINT)strtoul(argv[2], NULL, 0) : 0u;

#if defined(_WIN32)
    _setmode(_fileno(stdout), _O_BINARY); // no newline translation on Windows
#else
    signal(SIGPIPE, SIG_IGN); // exit cleanly when the reader closes the pipe
#endif

    uint64_t ctr = 0;

    if (strcmp(mode, "ctr32") == 0) {
        for (;;) {
            for (unsigned i = 0; i < BUFWORDS; ++i)
                g_buf[i] = mchr_get_1d_hash_uint((MCHR_INT)(MCHR_UINT)ctr++, seed);
            if (fwrite(g_buf, sizeof(MCHR_UINT), BUFWORDS, stdout) != BUFWORDS) return 0;
        }
    } else if (strcmp(mode, "seed") == 0) {
        MCHR_UINT s = 0;
        const MCHR_INT fixed_index = 1234567;
        for (;;) {
            for (unsigned i = 0; i < BUFWORDS; ++i)
                g_buf[i] = mchr_get_1d_hash_uint(fixed_index, s++);
            if (fwrite(g_buf, sizeof(MCHR_UINT), BUFWORDS, stdout) != BUFWORDS) return 0;
        }
    } else { /* ctr64 (default) */
        for (;;) {
            for (unsigned i = 0; i < BUFWORDS; ++i) {
                MCHR_INT lo = (MCHR_INT)(MCHR_UINT)ctr;
                MCHR_INT hi = (MCHR_INT)(MCHR_UINT)(ctr >> 32);
                g_buf[i] = mchr_get_2d_hash_uint(lo, hi, seed);
                ctr++;
            }
            if (fwrite(g_buf, sizeof(MCHR_UINT), BUFWORDS, stdout) != BUFWORDS) return 0;
        }
    }
    return 0;
}
