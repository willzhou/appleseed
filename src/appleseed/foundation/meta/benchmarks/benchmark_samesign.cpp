
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2012 Francois Beaune, Jupiter Jazz Limited
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// appleseed.foundation headers.
#include "foundation/math/rng.h"
#include "foundation/platform/compiler.h"
#ifdef APPLESEED_FOUNDATION_USE_SSE
#include "foundation/platform/sse.h"
#endif
#include "foundation/utility/benchmark.h"
#include "foundation/utility/casts.h"
#include "foundation/utility/otherwise.h"

// Standard headers.
#include <cmath>
#include <cstddef>

//
// Research for this post on Stack Overflow:
//
// "How to efficiently compare the sign of two floating-point values while handling negative zeros"
// http://stackoverflow.com/questions/2922619/how-to-efficiently-compare-the-sign-of-two-floating-point-values-while-handling-n
//

BENCHMARK_SUITE(SameSign)
{
    using namespace foundation;
    using namespace std;

    //
    // Empty function.
    //

    NO_INLINE bool empty_function(const float a, const float b)
    {
        return false;
    }

    NO_INLINE bool empty_function(const float a, const float b, const float c)
    {
        return false;
    }

    //
    // Naive variant.
    //

    NO_INLINE bool same_sign_naive(const float a, const float b)
    {
        if (fabs(a) == 0.0f || fabs(b) == 0.0f)
            return true;

        return (a >= 0.0f) == (b >= 0.0f);
    }

    NO_INLINE bool same_sign_naive(const float a, const float b, const float c)
    {
        return same_sign_naive(a, b) && same_sign_naive(a, c) && same_sign_naive(b, c);
    }

    //
    // Integer arithmetic-based variant.
    //

    NO_INLINE bool same_sign_integer(const float a, const float b)
    {
        const int ia = binary_cast<int>(a);
        const int ib = binary_cast<int>(b);

        const int az = (ia & 0x7FFFFFFF) == 0;
        const int bz = (ib & 0x7FFFFFFF) == 0;
        const int ab = (ia ^ ib) >= 0;

        return (az | bz | ab) != 0;
    }

    NO_INLINE bool same_sign_integer(const float a, const float b, const float c)
    {
        const int32 ia = binary_cast<int32>(a);
        const int32 ib = binary_cast<int32>(b);
        const int32 ic = binary_cast<int32>(c);

        const int32 az = (ia & 0x7FFFFFFFL) == 0;
        const int32 bz = (ib & 0x7FFFFFFFL) == 0;
        const int32 cz = (ic & 0x7FFFFFFFL) == 0;

        const int32 ab = (ia ^ ib) >= 0;
        const int32 ac = (ia ^ ic) >= 0;
        const int32 bc = (ib ^ ic) >= 0;

        const int32 b1 = ab | az | bz;
        const int32 b2 = ac | az | cz;
        const int32 b3 = bc | bz | cz;

        return (b1 & b2 & b3) != 0;
    }

    //
    // Multiplication-based variant.
    //

    NO_INLINE bool same_sign_multiplication(const float a, const float b)
    {
        return a * b >= 0.0f;
    }

    NO_INLINE bool same_sign_multiplication(const float a, const float b, const float c)
    {
        return a * b >= 0.0f && a * c >= 0.0f && b * c >= 0.0f;
    }

#ifdef APPLESEED_FOUNDATION_USE_SSE

    //
    // SSE implementation of the 3-component multiplication-based variant.
    //

    NO_INLINE bool same_sign_multiplication_sse(const float a, const float b, const float c)
    {
        SSE_ALIGN float u[4] = { a, a, b, c };

        const sse4f mu = loadps(u);
        const sse4f mv = shuffleps(mu, mu, _MM_SHUFFLE(2, 3, 3, 2));
        const sse4f product = mulps(mu, mv);
        const sse4f zero = set1ps(0.0f);
        const sse4f cmp = cmpgeps(product, zero);
        const int mask = movemaskps(cmp);

        return mask == 0xF;
    }

#endif

    struct Fixture
    {
        static const size_t InvocationCount = 100;
        static const size_t ValueCount = InvocationCount + 2;

        bool    m_result;
        float   m_values[ValueCount];

        Fixture()
          : m_result(false)
        {
            MersenneTwister rng;

            for (size_t i = 0; i < ValueCount; ++i)
            {
                switch (rand_int1(rng, 0, 3))
                {
                  case 0: m_values[i] = -1.0f; break;
                  case 1: m_values[i] = -0.0f; break;
                  case 2: m_values[i] = +0.0f; break;
                  case 3: m_values[i] = +1.0f; break;
                  assert_otherwise;
                }
            }
        }
    };

    BENCHMARK_CASE_F(EmptyFunction2, Fixture)
    {
        for (size_t i = 0; i < InvocationCount; ++i)
            m_result ^= empty_function(m_values[i], m_values[i + 1]);
    }

    BENCHMARK_CASE_F(EmptyFunction3, Fixture)
    {
        for (size_t i = 0; i < InvocationCount; ++i)
            m_result ^= empty_function(m_values[i], m_values[i + 1], m_values[i + 2]);
    }

    BENCHMARK_CASE_F(SameSignNaive2, Fixture)
    {
        for (size_t i = 0; i < InvocationCount; ++i)
            m_result ^= same_sign_naive(m_values[i], m_values[i + 1]);
    }

    BENCHMARK_CASE_F(SameSignNaive3, Fixture)
    {
        for (size_t i = 0; i < InvocationCount; ++i)
            m_result ^= same_sign_naive(m_values[i], m_values[i + 1], m_values[i + 2]);
    }

    BENCHMARK_CASE_F(SameSignInteger2, Fixture)
    {
        for (size_t i = 0; i < InvocationCount; ++i)
            m_result ^= same_sign_integer(m_values[i], m_values[i + 1]);
    }

    BENCHMARK_CASE_F(SameSignInteger3, Fixture)
    {
        for (size_t i = 0; i < InvocationCount; ++i)
            m_result ^= same_sign_integer(m_values[i], m_values[i + 1], m_values[i + 2]);
    }

    BENCHMARK_CASE_F(SameSignMultiplication2, Fixture)
    {
        for (size_t i = 0; i < InvocationCount; ++i)
            m_result ^= same_sign_multiplication(m_values[i], m_values[i + 1]);
    }

    BENCHMARK_CASE_F(SameSignMultiplication3, Fixture)
    {
        for (size_t i = 0; i < InvocationCount; ++i)
            m_result ^= same_sign_multiplication(m_values[i], m_values[i + 1], m_values[i + 2]);
    }

#ifdef APPLESEED_FOUNDATION_USE_SSE

    BENCHMARK_CASE_F(SameSignMultiplicationSSE3, Fixture)
    {
        for (size_t i = 0; i < InvocationCount; ++i)
            m_result ^= same_sign_multiplication_sse(m_values[i], m_values[i + 1], m_values[i + 2]);
    }

#endif
}
