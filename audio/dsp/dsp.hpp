/* Copyright (c) 2017-2018 Hans-Kristian Arntzen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <cmath>

#ifdef _WIN32
#define __SSE__
#endif

#if defined(__SSE__)
#include <xmmintrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#endif

namespace Granite
{
namespace Audio
{
namespace DSP
{
static inline void accumulate_channel(float * __restrict output, const float * __restrict input, float gain, size_t count) noexcept
{
#ifdef __ARM_NEON
	size_t rounded_count = count & ~3;
	for (size_t i = 0; i < rounded_count; i += 4)
	{
		float32x4_t acc = vld1q_f32(output);
		float32x4_t in = vld1q_f32(input);
		acc = vfmaq_n_f32(acc, in, gain);
		vst1q_f32(output, acc);

		output += 4;
		input += 4;
	}

	for (size_t i = rounded_count; i < count; i++)
		output[i] += input[i] * gain;
#else
	for (size_t i = 0; i < count; i++)
		output[i] += input[i] * gain;
#endif
}

static int16_t f32_to_i16(float v) noexcept
{
	int32_t i = int32_t(std::round(v * 0x8000));
	if (i > 0x7fff)
		return 0x7fff;
	else if (i < -0x8000)
		return -0x8000;
	else
		return int16_t(i);
}

static inline void interleave_stereo_f32(float * __restrict target,
                                         const float * __restrict left,
                                         const float * __restrict right,
                                         size_t count) noexcept
{
#ifdef __SSE__
	size_t rounded_count = count & ~3;
	for (size_t i = 0; i < rounded_count; i += 4)
	{
		__m128 l = _mm_loadu_ps(left);
		__m128 r = _mm_loadu_ps(right);
		left += 4;
		right += 4;
		__m128 interleaved0 = _mm_unpacklo_ps(l, r);
		__m128 interleaved1 = _mm_unpackhi_ps(l, r);
		_mm_storeu_ps(target, interleaved0);
		_mm_storeu_ps(target + 4, interleaved1);
		target += 8;
	}

	for (size_t i = rounded_count; i < count; i++)
	{
		*target++ = *left++;
		*target++ = *right++;
	}
#else
	for (size_t i = 0; i < count; i++)
	{
		*target++ = *left++;
		*target++ = *right++;
	}
#endif
}

static inline void interleave_stereo_f32_i16(int16_t * __restrict target,
                                             const float * __restrict left,
                                             const float * __restrict right,
                                             size_t count) noexcept
{
#ifdef __ARM_NEON
	size_t rounded_count = count & ~3;
	for (size_t i = 0; i < rounded_count; i += 4)
	{
		float32x4_t l = vld1q_f32(left);
		float32x4_t r = vld1q_f32(right);

		l = vmulq_n_f32(l, float(0x8000));
		r = vmulq_n_f32(r, float(0x8000));

		int32x4_t il = vcvtq_s32_f32(l);
		int32x4_t ir = vcvtq_s32_f32(r);
		int16x4_t sl = vqmovn_s32(il);
		int16x4_t sr = vqmovn_s32(ir);
		int16x4x2_t stereo = { sl, sr };
		vst2_s16(target, stereo);

		left += 4;
		right += 4;
		target += 8;
	}

	for (size_t i = rounded_count; i < count; i++)
	{
		*target++ = f32_to_i16(*left++);
		*target++ = f32_to_i16(*right++);
	}
#else
	for (size_t i = 0; i < count; i++)
	{
		*target++ = f32_to_i16(*left++);
		*target++ = f32_to_i16(*right++);
	}
#endif
}
}
}
}