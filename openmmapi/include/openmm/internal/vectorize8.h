#ifndef OPENMM_VECTORIZE8_H_
#define OPENMM_VECTORIZE8_H_

/* -------------------------------------------------------------------------- *
 *                                   OpenMM                                   *
 * -------------------------------------------------------------------------- *
 * This is part of the OpenMM molecular simulation toolkit originating from   *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2013-2014 Stanford University and the Authors.      *
 * Authors: Peter Eastman                                                     *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

#include "vectorize.h"
#include <immintrin.h>

// This file defines classes and functions to simplify vectorizing code with AVX.

class ivec8;

/**
 * An eight element vector of floats.
 */
class fvec8 {
public:
    __m256 val;

    fvec8() {}
    fvec8(float v) : val(_mm256_set1_ps(v)) {}
    fvec8(float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8) : val(_mm256_set_ps(v8, v7, v6, v5, v4, v3, v2, v1)) {}
    fvec8(__m256 v) : val(v) {}
    fvec8(const float* v) : val(_mm256_loadu_ps(v)) {}
    operator __m256() const {
        return val;
    }
    fvec4 lowerVec() const {
        return _mm256_castps256_ps128(val);
    }
    fvec4 upperVec() const {
        return _mm256_extractf128_ps(val, 1);
    }
    void store(float* v) const {
        _mm256_storeu_ps(v, val);
    }
    fvec8 operator+(const fvec8& other) const {
        return _mm256_add_ps(val, other);
    }
    fvec8 operator-(const fvec8& other) const {
        return _mm256_sub_ps(val, other);
    }
    fvec8 operator*(const fvec8& other) const {
        return _mm256_mul_ps(val, other);
    }
    fvec8 operator/(const fvec8& other) const {
        return _mm256_div_ps(val, other);
    }
    void operator+=(const fvec8& other) {
        val = _mm256_add_ps(val, other);
    }
    void operator-=(const fvec8& other) {
        val = _mm256_sub_ps(val, other);
    }
    void operator*=(const fvec8& other) {
        val = _mm256_mul_ps(val, other);
    }
    void operator/=(const fvec8& other) {
        val = _mm256_div_ps(val, other);
    }
    fvec8 operator-() const {
        return _mm256_sub_ps(_mm256_set1_ps(0.0f), val);
    }
    fvec8 operator&(const fvec8& other) const {
        return _mm256_and_ps(val, other);
    }
    fvec8 operator|(const fvec8& other) const {
        return _mm256_or_ps(val, other);
    }
    fvec8 operator==(const fvec8& other) const {
        return _mm256_cmp_ps(val, other, _CMP_EQ_OQ);
    }
    fvec8 operator!=(const fvec8& other) const {
        return _mm256_cmp_ps(val, other, _CMP_NEQ_OQ);
    }
    fvec8 operator>(const fvec8& other) const {
        return _mm256_cmp_ps(val, other, _CMP_GT_OQ);
    }
    fvec8 operator<(const fvec8& other) const {
        return _mm256_cmp_ps(val, other, _CMP_LT_OQ);
    }
    fvec8 operator>=(const fvec8& other) const {
        return _mm256_cmp_ps(val, other, _CMP_GE_OQ);
    }
    fvec8 operator<=(const fvec8& other) const {
        return _mm256_cmp_ps(val, other, _CMP_LE_OQ);
    }
    operator ivec8() const;
};

static inline int8_t getMaskFromCompare(fvec8 compare_result) {
    return _mm256_movemask_ps(compare_result);
}

/**
 * An eight element vector of ints.
 */
class ivec8 {
public:
    __m256i val;

    ivec8() {}
    ivec8(int v) : val(_mm256_set1_epi32(v)) {}
    ivec8(int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8) : val(_mm256_set_epi32(v8, v7, v6, v5, v4, v3, v2, v1)) {}
    ivec8(__m256i v) : val(v) {}
    ivec8(const int* v) : val(_mm256_loadu_si256((const __m256i*) v)) {}
    operator __m256i() const {
        return val;
    }
    ivec4 lowerVec() const {
        return _mm256_castsi256_si128(val);
    }
    ivec4 upperVec() const {
        return _mm256_extractf128_si256(val, 1);
    }
    void store(int* v) const {
        _mm256_storeu_si256((__m256i*) v, val);
    }
    ivec8 operator&(const ivec8& other) const {
        return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(val), _mm256_castsi256_ps(other.val)));
    }
    ivec8 operator|(const ivec8& other) const {
        return _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(val), _mm256_castsi256_ps(other.val)));
    }
    operator fvec8() const;
};

// Conversion operators.

inline fvec8::operator ivec8() const {
    return _mm256_cvttps_epi32(val);
}

inline ivec8::operator fvec8() const {
    return _mm256_cvtepi32_ps(val);
}

// Functions that operate on fvec8s.

static inline fvec8 floor(const fvec8& v) {
    return fvec8(_mm256_round_ps(v.val, 0x09));
}

static inline fvec8 ceil(const fvec8& v) {
    return fvec8(_mm256_round_ps(v.val, 0x0A));
}

static inline fvec8 round(const fvec8& v) {
    return fvec8(_mm256_round_ps(v.val, _MM_FROUND_TO_NEAREST_INT));
}

static inline fvec8 min(const fvec8& v1, const fvec8& v2) {
    return fvec8(_mm256_min_ps(v1.val, v2.val));
}

static inline fvec8 max(const fvec8& v1, const fvec8& v2) {
    return fvec8(_mm256_max_ps(v1.val, v2.val));
}

static inline fvec8 abs(const fvec8& v) {
    static const __m256 mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
    return fvec8(_mm256_and_ps(v.val, mask));
}

static inline fvec8 sqrt(const fvec8& v) {
    return fvec8(_mm256_sqrt_ps(v.val));
}

static inline fvec8 rsqrt(const fvec8& v) {
    // Initial estimate of rsqrt().

    fvec8 y(_mm256_rsqrt_ps(v.val));

    // Perform an iteration of Newton refinement.

    fvec8 x2 = v*0.5f;
    y *= fvec8(1.5f)-x2*y*y;
    return y;
}

static inline float dot8(const fvec8& v1, const fvec8& v2) {
    fvec8 result = _mm256_dp_ps(v1, v2, 0xF1);
    return _mm_cvtss_f32(result.lowerVec())+_mm_cvtss_f32(result.upperVec());
}

static inline void transpose(const fvec4& in1, const fvec4& in2, const fvec4& in3, const fvec4& in4, const fvec4& in5, const fvec4& in6, const fvec4& in7, const fvec4& in8, fvec8& out1, fvec8& out2, fvec8& out3, fvec8& out4) {
    fvec4 i1 = in1, i2 = in2, i3 = in3, i4 = in4;
    fvec4 i5 = in5, i6 = in6, i7 = in7, i8 = in8;
    _MM_TRANSPOSE4_PS(i1, i2, i3, i4);
    _MM_TRANSPOSE4_PS(i5, i6, i7, i8);
#ifdef _MSC_VER
    // Visual Studio has a bug in _mm256_castps128_ps256, so we have to use the more expensive _mm256_insertf128_ps.
    out1 = _mm256_insertf128_ps(out1, i1, 0);
    out2 = _mm256_insertf128_ps(out2, i2, 0);
    out3 = _mm256_insertf128_ps(out3, i3, 0);
    out4 = _mm256_insertf128_ps(out4, i4, 0);
#else
    out1 = _mm256_castps128_ps256(i1);
    out2 = _mm256_castps128_ps256(i2);
    out3 = _mm256_castps128_ps256(i3);
    out4 = _mm256_castps128_ps256(i4);
#endif
    out1 = _mm256_insertf128_ps(out1, i5, 1);
    out2 = _mm256_insertf128_ps(out2, i6, 1);
    out3 = _mm256_insertf128_ps(out3, i7, 1);
    out4 = _mm256_insertf128_ps(out4, i8, 1);
}

static inline void transpose(const fvec8& in1, const fvec8& in2, const fvec8& in3, const fvec8& in4, fvec4& out1, fvec4& out2, fvec4& out3, fvec4& out4, fvec4& out5, fvec4& out6, fvec4& out7, fvec4& out8) {
    out1 = in1.lowerVec();
    out2 = in2.lowerVec();
    out3 = in3.lowerVec();
    out4 = in4.lowerVec();
    _MM_TRANSPOSE4_PS(out1, out2, out3, out4);
    out5 = in1.upperVec();
    out6 = in2.upperVec();
    out7 = in3.upperVec();
    out8 = in4.upperVec();
    _MM_TRANSPOSE4_PS(out5, out6, out7, out8);
}

// Functions that operate on ivec8s.

static inline bool any(const ivec8& v) {
    return !_mm256_testz_si256(v, _mm256_set1_epi32(0xFFFFFFFF));
}

// Mathematical operators involving a scalar and a vector.

static inline fvec8 operator+(float v1, const fvec8& v2) {
    return fvec8(v1)+v2;
}

static inline fvec8 operator-(float v1, const fvec8& v2) {
    return fvec8(v1)-v2;
}

static inline fvec8 operator*(float v1, const fvec8& v2) {
    return fvec8(v1)*v2;
}

static inline fvec8 operator/(float v1, const fvec8& v2) {
    return fvec8(v1)/v2;
}

// Operations for blending fvec8s from either a full bitmask or an 8-bit mask.

static inline fvec8 blend(const fvec8& v1, const fvec8& v2, const ivec8& mask) {
    return fvec8(_mm256_blendv_ps(v1.val, v2.val, _mm256_castsi256_ps(mask.val)));
}

static inline fvec8 blend(const fvec8& v1, const fvec8& v2, int8_t mask)
{
   // Put a copy of bit 0 in the first element, bit 1 in the second, and so on.
    const ivec8 expandedBits = _mm256_set1_epi8(mask) & _mm256_setr_epi32(1, 2, 4, 8, 16, 32, 64, 128);

    // The individual bits are essentially extremely small floating-point values. By comparing against zero
    //  (even a floating-point zero), the individual bits are turned into a complete element mask.
    const auto elementMask = _mm256_cmp_ps(_mm256_castsi256_ps(expandedBits), __m256(), _CMP_NEQ_OQ);

    return _mm256_blendv_ps(v1, v2, elementMask);
}

/// Given a table of floating-point values and a set of indexes, perform a gather read into a pair
/// of vectors. The first result vector contains the values at the given indexes, and the second
/// result vector contains the values from each respective index+1.
static inline void gatherVecPair(const float* table, const ivec8 index, fvec8& out0, fvec8& out1)
{
    // Utility function to read a pair of values from the given table index and broadcast the pair to
    // every element of the vector.
    auto broadcast2ps = [&](int32_t i) -> __m256 {
        return _mm256_castpd_ps(_mm256_broadcast_sd((double*)(table + i)));
    };

    // Extract the 8 index positions as 4 pairs of 64-bit values. Extracting a scalar value
    // from a vector is relatively expensive, so rather than pull out out individual 32-bit values,
    // pull out pairs of values as 64-bits and use the cheaper scalar instructions to
    // manipulate the address.
    int64_t pairAddr0 = _mm256_extract_epi64(index, 0);
    int64_t pairAddr1 = _mm256_extract_epi64(index, 1);
    int64_t pairAddr2 = _mm256_extract_epi64(index, 2);
    int64_t pairAddr3 = _mm256_extract_epi64(index, 3);

    // Gather the data from the individual reads. Each read will return a pair of floating-point
    // values from that index position. The first values from each read need to be stored in
    // the first result vector, and the second value in the second result vector. Broadcast the pairs
    // to every element position in a vector and then use the cheap blend instructions to merge the results together.
    const auto a0 = broadcast2ps(pairAddr0 & 0xFFFFFFFF);
    const auto b1 = broadcast2ps(pairAddr0 >> 32);
    const auto c2 = broadcast2ps(pairAddr1 & 0xFFFFFFFF);
    const auto d3 = broadcast2ps(pairAddr1 >> 32);
    const auto e4 = broadcast2ps(pairAddr2 & 0xFFFFFFFF);
    const auto f5 = broadcast2ps(pairAddr2 >> 32);
    const auto g6 = broadcast2ps(pairAddr3 & 0xFFFFFFFF);
    const auto h7 = broadcast2ps(pairAddr3 >> 32);

    const auto a0b1 = _mm256_blend_ps(a0, b1, 0b11001100);
    const auto c2d3 = _mm256_blend_ps(c2, d3, 0b11001100);
    const auto e4f5 = _mm256_blend_ps(e4, f5, 0b11001100);
    const auto g6h7 = _mm256_blend_ps(g6, h7, 0b11001100);

    const auto a0b1e4f5 = _mm256_blend_ps(a0b1, e4f5, 0b11110000);
    const auto c2d3g6h7 = _mm256_blend_ps(c2d3, g6h7, 0b11110000);

    out0 = _mm256_shuffle_ps(a0b1e4f5, c2d3g6h7, 0b10001000);
    out1 = _mm256_shuffle_ps(a0b1e4f5, c2d3g6h7, 0b11011101);
}

/// Given 3 vectors of floating-point data, reduce them to a single 3-element position
/// value by adding all the elements in each vector. Given inputs of:
///   X0 X1 X2 X3 X4 X5 X6 X7
///   Y0 Y1 Y2 Y3 Y4 Y5 Y6 Y7
///   Z0 Z1 Z2 Z3 Z4 Z5 Z6 Z7
/// Each vector of values needs to be summed into a single value, and then stored into
/// the output vector:
///   output[0] = (X0 + X1 + X2 + ...)
///   output[1] = (Y0 + Y1 + Y2 + ...)
///   output[2] = (Z0 + Z1 + Z2 + ...)
///   output[3] = undefined
static inline fvec4 reduceToVec3(const fvec8 x, const fvec8 y, const fvec8 z)
{
    // The general strategy for a vector reduce-add operation is to take values from
    // different parts of the vector and overlap them a different part of the vector and then
    // add together. Repeat this several times until all values have been summed. Initially 8
    // values can be reduced to 4, 4 to 2, and 2 to 1. The following code essentially does this
    // but exploits two things:
    //   - having multiple inputs means that some vectors can be combined together to amortise the
    //     cost of shuffling.
    //   - the output destinations are part of anther vector, so accumulate into the correct
    //     offsets to start with, instead of reducing to position 0 and re-inserting to the correct
    //     output location.
    //
    // As far as possible, accumulate x, y and z into their output positions in both the top and
    // bottom 128-bits to exploit in-lane permutes as much as possible early on.

    // Shuffle X and Z together to form one reduced vector.
    //   X2 X3 Z0 Z1 X6 X7 Z4 Z5
    const auto xzshuf = _mm256_shuffle_ps(x, z, 0b01001110);
    // Blend X and Z together to form another reduced vector, overlapping the previous.
    //   X0 X1 Z2 Z3 X4 X5 Z6 Z7
    const auto xzblend = _mm256_blend_ps(x, z, 0b11001100);
    // Add them together to form:
    // (X0 + X2) (X1 + X3) (Z0 + Z2) (Z1 + Z3) etc.
    const auto xz0 = _mm256_add_ps(xzshuf, xzblend);

    // Now there's only one vector containing all values. Shuffle again to form another overlap,
    // and then add.
    const auto xz1 = _mm256_permute_ps(xz0, 0b00110001);
    const auto xz2 = _mm256_add_ps(xz0, xz1);

    // Work on Z on its own as there's nothing else to work with. Start by permuting it to
    // form some overlaps, and then add:
    //   (Y0 + Y2) (Y1 + Y3) - - (Y4 + Y6) (Y5 + Y7) - -
    const auto yshuf = _mm256_permute_ps(y, 0b11101110);
    const auto y0 = _mm256_add_ps(yshuf, y);

    // Shift the bottom float of each pair to the right, into the correct Y location.
    const auto y1 = _mm256_permute_ps(y0, 0b00000000);
    const auto y2 = _mm256_add_ps(y0, y1);

    // Blend the results together to give a complete set of XYZ in the correct respective positions
    // of both top and bottom 128-bit lanes.
    const auto laneResult = fvec8(_mm256_blend_ps(xz2, y2, 0b00100010));

    return laneResult.lowerVec() + laneResult.upperVec();
}

#endif /*OPENMM_VECTORIZE8_H_*/
