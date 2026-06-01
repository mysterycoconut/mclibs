// test_mc_hash_rng.c - unit tests for mc_hash_rng v1.0, using greatest.
//
//   build: cc -O2 -std=c11 -Wall -Wextra -I.. test_mc_hash_rng.c -o test_mchr -lm
//   run:   ./test_mchr (add -v for per-test output)
//
// This is a white-box test: it #defines MCHR_IMPLEMENTATION so it can also reach the
//  static priv mappers to check the exact float endpoints. It exercises every public
//  helper and each property the header comments assert.

#include <math.h>
#include <limits.h>

#define MCHR_IMPLEMENTATION
#define MCHR_USE_STDINT // exercise the stdint typedefs; behaviour is identical either way
#include "mc_hash_rng.h"

#include "greatest.h"
GREATEST_MAIN_DEFS();

static const MCHR_UINT SEEDS[] = { 0u, 1u, 7u, 12345u, 0xDEADBEEFu, 0xFFFFFFFFu };
#define NSEEDS ((int)(sizeof(SEEDS) / sizeof(SEEDS[0])))

// --------------------------------------------------------------------------------------
// Determinism: every public helper returns the same value when called twice (this also
//  confirms each of the 31 functions links and runs).
// --------------------------------------------------------------------------------------
TEST determinism_all_helpers(void) {
    for (int s = 0; s < NSEEDS; ++s) {
        MCHR_UINT sd = SEEDS[s];
        MCHR_INT a = 11, b = -22, c = 333, d = -4444;
        MCHR_INT buf[3] = { a, b, c };
        size_t bl = sizeof(buf);

        ASSERT(mchr_get_hash_uint(buf, bl, sd) == mchr_get_hash_uint(buf, bl, sd));
        ASSERT(mchr_get_1d_hash_uint(a, sd) == mchr_get_1d_hash_uint(a, sd));
        ASSERT(mchr_get_2d_hash_uint(a, b, sd) == mchr_get_2d_hash_uint(a, b, sd));
        ASSERT(mchr_get_3d_hash_uint(a, b, c, sd) == mchr_get_3d_hash_uint(a, b, c, sd));
        ASSERT(mchr_get_4d_hash_uint(a, b, c, d, sd) == mchr_get_4d_hash_uint(a, b, c, d, sd));

        ASSERT(mchr_get_hash_uint_under_limit(buf, bl, sd, 1000u)
            == mchr_get_hash_uint_under_limit(buf, bl, sd, 1000u));

        ASSERT(mchr_get_hash_uint_in_range(buf, bl, sd, 5u, 9u) == mchr_get_hash_uint_in_range(buf, bl, sd, 5u, 9u));
        ASSERT(mchr_get_1d_hash_uint_in_range(a, sd, 5u, 9u) == mchr_get_1d_hash_uint_in_range(a, sd, 5u, 9u));
        ASSERT(mchr_get_2d_hash_uint_in_range(a, b, sd, 5u, 9u)== mchr_get_2d_hash_uint_in_range(a, b, sd, 5u, 9u));
        ASSERT(mchr_get_3d_hash_uint_in_range(a, b, c, sd, 5u, 9u)== mchr_get_3d_hash_uint_in_range(a, b, c, sd, 5u, 9u));
        ASSERT(mchr_get_4d_hash_uint_in_range(a, b, c, d, sd, 5u, 9u) == mchr_get_4d_hash_uint_in_range(a, b, c, d, sd, 5u, 9u));

        ASSERT(mchr_get_hash_int_in_range(buf, bl, sd, -5, 9) == mchr_get_hash_int_in_range(buf, bl, sd, -5, 9));
        ASSERT(mchr_get_1d_hash_int_in_range(a, sd, -5, 9) == mchr_get_1d_hash_int_in_range(a, sd, -5, 9));
        ASSERT(mchr_get_2d_hash_int_in_range(a, b, sd, -5, 9) == mchr_get_2d_hash_int_in_range(a, b, sd, -5, 9));
        ASSERT(mchr_get_3d_hash_int_in_range(a, b, c, sd, -5, 9) == mchr_get_3d_hash_int_in_range(a, b, c, sd, -5, 9));
        ASSERT(mchr_get_4d_hash_int_in_range(a, b, c, d, sd, -5, 9) == mchr_get_4d_hash_int_in_range(a, b, c, d, sd, -5, 9));

        ASSERT(mchr_get_hash_zero_to_one(buf, bl, sd) == mchr_get_hash_zero_to_one(buf, bl, sd));
        ASSERT(mchr_get_1d_hash_zero_to_one(a, sd) == mchr_get_1d_hash_zero_to_one(a, sd));
        ASSERT(mchr_get_2d_hash_zero_to_one(a, b, sd) == mchr_get_2d_hash_zero_to_one(a, b, sd));
        ASSERT(mchr_get_3d_hash_zero_to_one(a, b, c, sd) == mchr_get_3d_hash_zero_to_one(a, b, c, sd));
        ASSERT(mchr_get_4d_hash_zero_to_one(a, b, c, d, sd) == mchr_get_4d_hash_zero_to_one(a, b, c, d, sd));

        ASSERT(mchr_get_hash_neg_one_to_one(buf, bl, sd) == mchr_get_hash_neg_one_to_one(buf, bl, sd));
        ASSERT(mchr_get_1d_hash_neg_one_to_one(a, sd) == mchr_get_1d_hash_neg_one_to_one(a, sd));
        ASSERT(mchr_get_2d_hash_neg_one_to_one(a, b, sd) == mchr_get_2d_hash_neg_one_to_one(a, b, sd));
        ASSERT(mchr_get_3d_hash_neg_one_to_one(a, b, c, sd) == mchr_get_3d_hash_neg_one_to_one(a, b, c, sd));
        ASSERT(mchr_get_4d_hash_neg_one_to_one(a, b, c, d, sd) == mchr_get_4d_hash_neg_one_to_one(a, b, c, d, sd));

        ASSERT(mchr_get_chance(buf, bl, sd, 0.5f) == mchr_get_chance(buf, bl, sd, 0.5f));
        ASSERT(mchr_get_1d_chance(a, sd, 0.5f) == mchr_get_1d_chance(a, sd, 0.5f));
        ASSERT(mchr_get_2d_chance(a, b, sd, 0.5f) == mchr_get_2d_chance(a, b, sd, 0.5f));
        ASSERT(mchr_get_3d_chance(a, b, c, sd, 0.5f) == mchr_get_3d_chance(a, b, c, sd, 0.5f));
        ASSERT(mchr_get_4d_chance(a, b, c, d, sd, 0.5f) == mchr_get_4d_chance(a, b, c, d, sd, 0.5f));
    }
    PASS();
}

// --------------------------------------------------------------------------------------
// Doc claim: the 1d-4d integer helpers equal the generic hash over an array of MCHR_INT.
// --------------------------------------------------------------------------------------
TEST nd_equals_generic_over_array(void) {
    MCHR_INT x = 130, y = -23, z = 9001, t = -4;
    for (int s = 0; s < NSEEDS; ++s) {
        MCHR_UINT sd = SEEDS[s];
        MCHR_INT a1[1] = { x }, a2[2] = { x, y }, a3[3] = { x, y, z }, a4[4] = { x, y, z, t };
        ASSERT(mchr_get_1d_hash_uint(x, sd) == mchr_get_hash_uint(a1, sizeof(a1), sd));
        ASSERT(mchr_get_2d_hash_uint(x, y, sd) == mchr_get_hash_uint(a2, sizeof(a2), sd));
        ASSERT(mchr_get_3d_hash_uint(x, y, z, sd) == mchr_get_hash_uint(a3, sizeof(a3), sd));
        ASSERT(mchr_get_4d_hash_uint(x, y, z, t, sd) == mchr_get_hash_uint(a4, sizeof(a4), sd));
        // equivalence carries through the derived families:
        ASSERT(mchr_get_2d_hash_uint_in_range(x, y, sd, 3u, 77u) == mchr_get_hash_uint_in_range(a2, sizeof(a2), sd, 3u, 77u));
        ASSERT(mchr_get_3d_hash_int_in_range(x, y, z, sd, -50, 50) == mchr_get_hash_int_in_range(a3, sizeof(a3), sd, -50, 50));
        ASSERT(mchr_get_4d_hash_zero_to_one(x, y, z, t, sd) == mchr_get_hash_zero_to_one(a4, sizeof(a4), sd));
        ASSERT(mchr_get_1d_hash_neg_one_to_one(x, sd) == mchr_get_hash_neg_one_to_one(a1, sizeof(a1), sd));
        ASSERT(mchr_get_2d_chance(x, y, sd, 0.5f) == mchr_get_chance(a2, sizeof(a2), sd, 0.5f));
    }
    PASS();
}

// --------------------------------------------------------------------------------------
// under_limit: result is always in [0, upper_bound); the two documented edge conventions.
// --------------------------------------------------------------------------------------
TEST under_limit_bounds_and_edges(void) {
    const MCHR_UINT bounds[] = { 2u, 6u, 7u, 100u, 1000u, 65537u, (1u<<24), (1u<<31), 0xFFFFFFFEu, 0xFFFFFFFFu };
    for (int bi = 0; bi < (int)(sizeof(bounds)/sizeof(bounds[0])); ++bi) {
        MCHR_UINT ub = bounds[bi];
        for (MCHR_INT i = 0; i < 20000; ++i) {
            MCHR_UINT r = mchr_get_hash_uint_under_limit(&i, sizeof(i), 1u, ub);
            ASSERT(r < ub);
        }
    }
    // upper_bound == 0 means "full 2^32 range" -> equals the raw hash
    for (MCHR_INT i = 0; i < 1000; ++i)
        ASSERT(mchr_get_hash_uint_under_limit(&i, sizeof(i), 9u, 0u) == mchr_get_hash_uint(&i, sizeof(i), 9u));
    // upper_bound == 1 -> only value in [0,1) is 0
    for (MCHR_INT i = 0; i < 1000; ++i)
        ASSERT(mchr_get_hash_uint_under_limit(&i, sizeof(i), 9u, 1u) == 0u);
    PASS();
}

// every value in a small [0,bound) is actually producible
TEST under_limit_coverage_small(void) {
    bool seen[6] = { false, false, false, false, false, false };
    for (MCHR_INT i = 0; i < 100000; ++i) {
        MCHR_UINT r = mchr_get_hash_uint_under_limit(&i, sizeof(i), 3u, 6u);
        seen[r] = true;
    }
    for (int v = 0; v < 6; ++v) ASSERTm("a value in [0,6) was never produced", seen[v]);
    PASS();
}

// --------------------------------------------------------------------------------------
// uint_in_range: closed [min,max]; min==max; full range; both endpoints reachable.
// --------------------------------------------------------------------------------------
TEST uint_in_range_bounds(void) {
    for (int s = 0; s < NSEEDS; ++s) {
        MCHR_UINT sd = SEEDS[s];
        for (MCHR_INT i = 0; i < 5000; ++i) {
            MCHR_UINT r = mchr_get_1d_hash_uint_in_range(i, sd, 10u, 20u);
            ASSERT(r >= 10u && r <= 20u);
        }
    }
    for (MCHR_INT i = 0; i < 1000; ++i) ASSERT(mchr_get_1d_hash_uint_in_range(i, 5u, 42u, 42u) == 42u); // min==max
    // full range (span wraps to the 0 == full-range convention) -- just exercise + determinism
    ASSERT(mchr_get_1d_hash_uint_in_range(7, 1u, 0u, 0xFFFFFFFFu) == mchr_get_1d_hash_uint_in_range(7, 1u, 0u, 0xFFFFFFFFu));
    PASS();
}

TEST uint_in_range_endpoints_reachable(void) {
    bool lo = false, hi = false;
    for (MCHR_INT i = 0; i < 200000 && !(lo && hi); ++i) {
        MCHR_UINT r = mchr_get_1d_hash_uint_in_range(i, 3u, 10u, 15u);
        if (r == 10u) lo = true;
        if (r == 15u) hi = true;
    }
    ASSERTm("min endpoint never reached", lo);
    ASSERTm("max endpoint never reached", hi);
    PASS();
}

// --------------------------------------------------------------------------------------
// int_in_range: closed [min,max], negative and very wide spans (no signed-overflow UB),
//  full INT range, min==max, endpoints reachable.
// --------------------------------------------------------------------------------------
TEST int_in_range_bounds_and_spans(void) {
    for (int s = 0; s < NSEEDS; ++s) {
        MCHR_UINT sd = SEEDS[s];
        for (MCHR_INT i = 0; i < 5000; ++i) {
            MCHR_INT r = mchr_get_1d_hash_int_in_range(i, sd, -7, 7);
            ASSERT(r >= -7 && r <= 7);
        }
    }
    for (MCHR_INT i = 0; i < 5000; ++i) { // span wider than INT_MAX
        MCHR_INT r = mchr_get_1d_hash_int_in_range(i, 2u, -2000000000, 2000000000);
        ASSERT(r >= -2000000000 && r <= 2000000000);
    }
    // full int range: span (uint)max-(uint)min+1 wraps to 0 -> full range; every int is in [INT_MIN,INT_MAX]
    ASSERT(mchr_get_1d_hash_int_in_range(9, 1u, INT_MIN, INT_MAX) == mchr_get_1d_hash_int_in_range(9, 1u, INT_MIN, INT_MAX));
    for (MCHR_INT i = 0; i < 1000; ++i) ASSERT(mchr_get_1d_hash_int_in_range(i, 5u, -13, -13) == -13); // min==max
    PASS();
}

TEST int_in_range_endpoints_reachable(void) {
    bool lo = false, hi = false;
    for (MCHR_INT i = 0; i < 200000 && !(lo && hi); ++i) {
        MCHR_INT r = mchr_get_1d_hash_int_in_range(i, 4u, -3, 3);
        if (r == -3) lo = true;
        if (r == 3)  hi = true;
    }
    ASSERT(lo && hi);
    PASS();
}

// --------------------------------------------------------------------------------------
// zero_to_one: closed [0,1], values on the 1/2^24 grid, exact closed endpoints, and the
//  documented balance around 0.5.
// --------------------------------------------------------------------------------------
TEST zero_to_one_range_and_grid(void) {
    const float scale = (float)(1 << 24);
    for (int s = 0; s < NSEEDS; ++s) {
        MCHR_UINT sd = SEEDS[s];
        for (MCHR_INT i = 0; i < 20000; ++i) {
            float v = mchr_get_1d_hash_zero_to_one(i, sd);
            ASSERT(v >= 0.0f && v <= 1.0f);
            float g = v * scale; // must land on an integer multiple of 1/2^24
            ASSERT(fabsf(g - roundf(g)) < 1.0e-2f);
        }
    }
    PASS();
}

TEST zero_to_one_endpoints_exact(void) {
    // white-box: the [0,2^24] -> [0,1] mapper hits both closed endpoints exactly,
    //  and under_limit's [0, 2^24] range can supply num = 0 and num = 2^24.
    ASSERT(mchr_priv_uint_to_zero_one(0u) == 0.0f);
    ASSERT(mchr_priv_uint_to_zero_one(1u << 24) == 1.0f);
    PASS();
}

TEST zero_to_one_balanced_around_half(void) {
    const int N = 1000000;
    long lo = 0;
    for (MCHR_INT i = 0; i < N; ++i) if (mchr_get_1d_hash_zero_to_one(i, 321u) < 0.5f) ++lo;
    double frac = (double)lo / N;
    ASSERTm("not ~balanced around 0.5", fabs(frac - 0.5) < 0.01);
    PASS();
}

// --------------------------------------------------------------------------------------
// neg_one_to_one: closed [-1,1], values on the 1/2^23 grid, exact closed endpoints.
// --------------------------------------------------------------------------------------
TEST neg_one_to_one_range_and_grid(void) {
    const float scale = (float)(1 << 23);
    for (int s = 0; s < NSEEDS; ++s) {
        MCHR_UINT sd = SEEDS[s];
        for (MCHR_INT i = 0; i < 20000; ++i) {
            float v = mchr_get_1d_hash_neg_one_to_one(i, sd);
            ASSERT(v >= -1.0f && v <= 1.0f);
            float g = (v + 1.0f) * scale; // multiple of 1/2^23
            ASSERT(fabsf(g - roundf(g)) < 1.0e-2f);
        }
    }
    // endpoints via the 2z-1 mapping at z's closed extremes
    ASSERT(2.0f * mchr_priv_uint_to_zero_one(0u)       - 1.0f == -1.0f);
    ASSERT(2.0f * mchr_priv_uint_to_zero_one(1u << 24) - 1.0f ==  1.0f);
    PASS();
}

// --------------------------------------------------------------------------------------
// chance: probability 0.0 is never true, 1.0 is always true, and the rate tracks p.
// --------------------------------------------------------------------------------------
TEST chance_zero_and_one(void) {
    for (int s = 0; s < NSEEDS; ++s) {
        MCHR_UINT sd = SEEDS[s];
        for (MCHR_INT i = 0; i < 20000; ++i) {
            ASSERT(mchr_get_1d_chance(i, sd, 0.0f) == false);
            ASSERT(mchr_get_1d_chance(i, sd, 1.0f) == true);
        }
    }
    PASS();
}

TEST chance_rate_tracks_probability(void) {
    const int N = 1000000;
    const float ps[] = { 0.25f, 0.5f, 0.75f };
    for (int pi = 0; pi < 3; ++pi) {
        float p = ps[pi];
        long cnt = 0;
        for (MCHR_INT i = 0; i < N; ++i) if (mchr_get_1d_chance(i, 777u, p)) ++cnt;
        double rate = (double)cnt / N;
        ASSERTm("chance rate off by >1%", fabs(rate - (double)p) < 0.01);
    }
    PASS();
}

// --------------------------------------------------------------------------------------
// Random access: the value at an index is independent of the order indices are queried.
// --------------------------------------------------------------------------------------
TEST random_access_order_independent(void) {
    enum { N = 64 };
    MCHR_UINT seq[N];
    for (int i = 0; i < N; ++i) seq[i] = mchr_get_1d_hash_uint((MCHR_INT)i, 555u);        // forward
    for (int i = N - 1; i >= 0; --i) ASSERT(mchr_get_1d_hash_uint((MCHR_INT)i, 555u) == seq[i]); // reverse
    int order[8] = { 5, 0, 51, 3, N - 1, 17, 2, 40 };                                     // out of order (doc's spirit)
    for (int k = 0; k < 8; ++k) ASSERT(mchr_get_1d_hash_uint((MCHR_INT)order[k], 555u) == seq[order[k]]);
    // and through a derived helper
    MCHR_UINT rseq[N];
    for (int i = 0; i < N; ++i) rseq[i] = mchr_get_1d_hash_uint_in_range((MCHR_INT)i, 555u, 0u, 99u);
    for (int i = N - 1; i >= 0; --i) ASSERT(mchr_get_1d_hash_uint_in_range((MCHR_INT)i, 555u, 0u, 99u) == rseq[i]);
    PASS();
}

// --------------------------------------------------------------------------------------
// Known-answer regression: locks the PractRand-validated v1.0 outputs against drift.
// --------------------------------------------------------------------------------------
TEST known_answer_regression(void) {
    ASSERT(mchr_get_1d_hash_uint(0, 12345u) == 0x923307cau);
    ASSERT(mchr_get_1d_hash_uint(1, 12345u) == 0xc6198c5au);
    ASSERT(mchr_get_1d_hash_uint(2, 12345u) == 0x1c4a632eu);
    ASSERT(mchr_get_1d_hash_uint(3, 12345u) == 0x4e3d8631u);
    PASS();
}

SUITE(mchr_contracts) {
    RUN_TEST(determinism_all_helpers);
    RUN_TEST(nd_equals_generic_over_array);
    RUN_TEST(under_limit_bounds_and_edges);
    RUN_TEST(under_limit_coverage_small);
    RUN_TEST(uint_in_range_bounds);
    RUN_TEST(uint_in_range_endpoints_reachable);
    RUN_TEST(int_in_range_bounds_and_spans);
    RUN_TEST(int_in_range_endpoints_reachable);
    RUN_TEST(zero_to_one_range_and_grid);
    RUN_TEST(zero_to_one_endpoints_exact);
    RUN_TEST(zero_to_one_balanced_around_half);
    RUN_TEST(neg_one_to_one_range_and_grid);
    RUN_TEST(chance_zero_and_one);
    RUN_TEST(chance_rate_tracks_probability);
    RUN_TEST(random_access_order_independent);
    RUN_TEST(known_answer_regression);
}

int main(int argc, char** argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(mchr_contracts);
    GREATEST_MAIN_END();
}
