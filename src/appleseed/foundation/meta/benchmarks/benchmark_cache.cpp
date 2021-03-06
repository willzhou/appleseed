
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
#include "foundation/utility/benchmark.h"
#include "foundation/utility/cache.h"

// Standard headers.
#include <cassert>
#include <cstddef>

using namespace foundation;
using namespace std;

BENCHMARK_SUITE(Foundation_Utility_Cache_LRUCache)
{
    template <size_t CacheSize>
    struct Fixture
    {
        typedef size_t MyKey;
        typedef int MyElement;

        struct MyElementSwapper
        {
            void load(const MyKey key, MyElement& element)
            {
                element = static_cast<MyElement>(key);
            }

            bool unload(const MyKey key, MyElement& element)
            {
                return true;
            }

            bool is_full(const size_t element_count) const
            {
                assert(element_count <= CacheSize);
                return element_count == CacheSize;
            }
        };

        MyElementSwapper                                m_element_swapper;
        LRUCache<MyKey, MyElement, MyElementSwapper>    m_cache;
        int                                             m_dummy;

        Fixture()
          : m_cache(m_element_swapper)
        {
        }

        void payload()
        {
            LCG rng;

            m_dummy = 0;

            for (size_t i = 0; i < 1000; ++i)
                m_dummy += m_cache.get(rand_int1(rng, 0, 99));
        }
    };

    BENCHMARK_CASE_F(LowHitRate, Fixture<5>)        { payload(); }
    BENCHMARK_CASE_F(MediumHitRate, Fixture<50>)    { payload(); }
    BENCHMARK_CASE_F(HighHitRate, Fixture<95>)      { payload(); }
}
