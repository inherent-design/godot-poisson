/**************************************************************************/
/*  big_random_pcg.h                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef BIG_RANDOM_PCG_H
#define BIG_RANDOM_PCG_H

#include "core/math/math_defs.h"
#include "core/typedefs.h"

#include "thirdparty/pcg-complete/pcg_random.hpp"

#include <math.h>
#include <random>

using namespace pcg_detail;

#define PCG_DEFAULT_SEED_128 0x853c49e6748fea9bULL
#define PCG_DEFAULT_INC_128 0xda3e39cb94b95bdbULL

#if defined(__GNUC__)
#define CLZ64(x) __builtin_clzll(x)
#elif defined(_MSC_VER)
#include <intrin.h>
static int __bsr_clz64(uint64_t x) {
	unsigned long index;
#if defined(_WIN64)
	_BitScanReverse64(&index, x);
#else
	uint32_t high = x >> 32;
	if (high != 0) {
		_BitScanReverse(&index, high);
		index += 32;
	} else {
		_BitScanReverse(&index, (uint32_t)x);
	}
#endif
	return 63 - index;
}
#define CLZ64(x) __bsr_clz64(x)
#else
#error "No CLZ64 implementation available for this compiler"
#endif

#if defined(__GNUC__)
#define LDEXP(s, e) __builtin_ldexp(s, e)
#define LDEXPF(s, e) __builtin_ldexpf(s, e)
#else
#include <math.h>
#define LDEXP(s, e) ldexp(s, e)
#define LDEXPF(s, e) ldexp(s, e)
#endif

template <typename T>
class Vector;

class BigRandomPCG {
	pcg64_fast pcg;
	pcg128_t initial_inc = PCG_DEFAULT_INC_128;
	pcg128_t current_seed = PCG_DEFAULT_SEED_128;
	pcg128_t current_inc = PCG_DEFAULT_INC_128;

public:
	BigRandomPCG(pcg128_t p_seed = PCG_DEFAULT_SEED_128, pcg128_t p_inc = PCG_DEFAULT_INC_128);

	// Seed management
	_FORCE_INLINE_ void seed(pcg128_t p_seed) {
		current_seed = p_seed;
		current_inc = initial_inc;

		pcg128_t seeds[2] = { current_seed, current_inc };
		std::seed_seq seed_seq(seeds, seeds + 2);

		pcg = pcg64_fast(seed_seq);
	}
	_FORCE_INLINE_ pcg128_t get_seed() { return current_seed; }

	// State management
	_FORCE_INLINE_ void set_state(pcg128_t p_state) {
		pcg.advance(-pcg.distance(0));
		pcg.advance(p_state);
	}
	_FORCE_INLINE_ pcg128_t get_state() const { return pcg.distance(0); }

	// Initialize
	void randomize();

	_FORCE_INLINE_ uint64_t rand() {
		return pcg();
	}
	_FORCE_INLINE_ uint64_t rand(uint64_t bounds) {
		return pcg(bounds);
	}
	int64_t rand_weighted(const Vector<float> &p_weights);
	uint64_t rand_poisson(uint64_t p_lambda);

	// Obtaining floating point numbers in [0, 1] range with "good enough" uniformity.
	// These functions sample the output of rand() as the fraction part of an infinite binary number,
	// with some tricks applied to reduce ops and branching:
	// 1. Instead of shifting to the first 1 and connecting random bits, we simply set the MSB and LSB to 1.
	//    Provided that the RNG is actually uniform bit by bit, this should have the exact same effect.
	// 2. In order to compensate for exponent info loss, we count zeros from another random number,
	//    and just add that to the initial offset.
	//    This has the same probability as counting and shifting an actual bit stream: 2^-n for n zeroes.
	// For all numbers above 2^-96 (2^-64 for floats), the functions should be uniform.
	// However, all numbers below that threshold are floored to 0.
	// The thresholds are chosen to minimize rand() calls while keeping the numbers within a totally subjective quality standard.
	// If clz or ldexp isn't available, fall back to bit truncation for performance, sacrificing uniformity.
	_FORCE_INLINE_ double randd() {
#if defined(CLZ64)
		uint64_t proto_exp_offset = rand();
		if (unlikely(proto_exp_offset == 0)) {
			return 0;
		}
		// Using 128-bit significand for better precision with pcg64
		pcg128_t significand = (((pcg128_t)rand()) << 64) | rand() | 0x8000000000000001ULL;
		return LDEXP((double)significand, -128 - CLZ64(proto_exp_offset));
#else
#pragma message("BigRandomPCG::randd - intrinsic clz is not available, falling back to bit truncation")
		return (double)(((((pcg128_t)rand()) << 64) | rand()) & 0x3FFFFFFFFFFFFFFFULL) / (double)0x3FFFFFFFFFFFFFFFULL;
#endif
	}
	_FORCE_INLINE_ float randf() {
#if defined(CLZ64)
		uint64_t proto_exp_offset = rand();
		if (unlikely(proto_exp_offset == 0)) {
			return 0;
		}
		return LDEXPF((float)(rand() | 0x8000000000000001ULL), -64 - CLZ64(proto_exp_offset));
#else
#pragma message("BigRandomPCG::randf - intrinsic clz is not available, falling back to bit truncation")
		return (float)(rand() & 0xFFFFFFFFFFFFFFULL) / (float)0xFFFFFFFFFFFFFFULL;
#endif
	}

	_FORCE_INLINE_ double randfn(double p_mean, double p_deviation) {
		double temp = randd();
		if (temp < CMP_EPSILON) {
			temp += CMP_EPSILON; // To prevent generating of INF value in log function, resulting to return NaN value from this function.
		}
		return p_mean + p_deviation * (cos(Math_TAU * randd()) * sqrt(-2.0 * log(temp))); // Box-Muller transform.
	}
	_FORCE_INLINE_ float randfn(float p_mean, float p_deviation) {
		float temp = randf();
		if (temp < CMP_EPSILON) {
			temp += CMP_EPSILON; // To prevent generating of INF value in log function, resulting to return NaN value from this function.
		}
		return p_mean + p_deviation * (cos((float)Math_TAU * randf()) * sqrt(-2.0 * log(temp))); // Box-Muller transform.
	}

	double random(double p_from, double p_to);
	float random(float p_from, float p_to);
	int random(int p_from, int p_to);

	uint64_t operator()() { return rand(); }
};

#endif // RANDOM_PCG_H
