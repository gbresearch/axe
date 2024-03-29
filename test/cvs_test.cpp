//-----------------------------------------------------------------------------
//  Copyright (C) 2011-2022, Gene Bushuyev
//  
//  Boost Software License - Version 1.0 - August 17th, 2003
//
//  Permission is hereby granted, free of charge, to any person or organization
//  obtaining a copy of the software and accompanying documentation covered by
//  this license (the "Software") to use, reproduce, display, distribute,
//  execute, and transmit the Software, and to prepare derivative works of the
//  Software, and to permit third-parties to whom the Software is furnished to
//  do so, all subject to the following:
//
//  The copyright notices in the Software and this entire statement, including
//  the above license grant, this restriction and the following disclaimer,
//  must be included in all copies of the Software, in whole or in part, and
//  all derivative works of the Software, unless such copies or derivative
//  works are solely in the form of machine-executable object code generated by
//  a source language processor.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
//  SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
//  FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include <string>
#include <sstream>
#include "../include/axe.h"
#include <yadro/util/gbtest.h>

using namespace axe;
using namespace shortcuts;

auto csv(const std::string& text)
{
    std::stringstream ss;
    // comma separator including trailing spaces
    auto cvs_separator = *_hs & ',';
    // rule for comma separated value
    auto csv_value = *_hs & +(_ - cvs_separator - _endl)
        >> [&](auto i1, auto i2)
    {
        ss << "<" << std::string(i1, i2) << ">";
    };

    // rule for single string of comma separated values
    auto line = csv_value % cvs_separator
        & _endl >> [&] { ss << "\n"; };

    // file contaning multiple csv lines
    auto csv_file = +line & _z
        | axe::r_fail([&](auto i1, auto i2) 
        {
            ss << "\nFailed: " << std::string(i1, i2);
        });

    parse(csv_file, text);
    return ss;
}

namespace
{
    using namespace gb::yadro::util;

    GB_TEST(axe, test_cvs)
    {
        auto ss = csv(R"*(Year, Make, Model, Trim, Length 
2010,Ford,E350, Wagon Passenger Van ,212.0
2011 , Toyota, Tundra, CREWMAX, 228.7 )*");
        
        gbassert(ss.str() == R"*(<Year><Make><Model><Trim><Length>
<2010><Ford><E350><Wagon Passenger Van><212.0>
<2011><Toyota><Tundra><CREWMAX><228.7>
)*");
    }
}