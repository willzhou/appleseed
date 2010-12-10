
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010 Francois Beaune
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
#include "foundation/platform/datetime.h"
#include "foundation/utility/string.h"
#include "foundation/utility/test.h"

// boost headers.
#include "boost/date_time/posix_time/posix_time.hpp"

// Standard headers.
#include <string>

using namespace boost;
using namespace foundation;
using namespace std;

TEST_SUITE(Foundation_Platform_DateTime)
{
    TEST_CASE(MicrosecondsToTimeDuration)
    {
        using namespace posix_time;

        const time_duration Expected(17, 29, 43, 1234);

        const time_duration result(
            microseconds_to_time_duration(Expected.total_microseconds()));

        EXPECT_EQ(Expected, result);
    }

    TEST_CASE(PTimeToString)
    {
        using namespace posix_time;

        const string Expected = "20100622T174531";

        const string result = to_string(from_iso_string(Expected));

        EXPECT_EQ(Expected, result);
    }
}