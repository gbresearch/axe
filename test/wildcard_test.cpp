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

#include <stdexcept>
#include <sstream>
#include <yadro/util/gbtest.h>

#include "../include/axe.h"


auto wildcard(const std::string& target)
{
    using namespace axe::shortcuts;
    using I = decltype(target.begin());
    axe::r_rule<I> result;

    // match any number of chars except '*'
    auto match_string = +(_ - '*');

    // make a rule to match an extracted string
    auto make_str_rule = [](auto i1, auto i2)
    {
        return axe::r_str(std::string(i1, i2));
    };

    // match wildcard starting with '*'
    auto first_rule = match_string >> [&](auto i1, auto i2)
    {
        result = make_str_rule(i1, i2);
    };

    // match wilcard starting with '*' followed by a string
    auto second_rule = "*"_axe & match_string >> [&](auto i1, auto i2)
    {
        result = result & axe::r_find(make_str_rule(i1, i2));
    };

    // match wildcard ending with '*'
    auto third_rule = "*"_axe >> [&]
    {
        result = result & *_;
    };

    auto wc_rule = first_rule & *second_rule & ~third_rule
        | +second_rule & ~third_rule
        | third_rule;

    if (!parse(wc_rule & _z, target).matched)
        throw std::runtime_error("invalid wildcard: " + target);

    return result;
}

namespace
{
    using namespace gb::yadro::util;

    GB_TEST(axe, test_wildcard)
    {
        auto grep = [](auto& wc, auto& text)
        {
            std::ostringstream ss;

            ss << ">grep \"" << wc << "\"\n";
            auto print = [&](auto i1, auto i2)
            {
                ss << std::string(i1, i2) << "\n";
            };

            // find all substrings matching the wildcards
            auto res = axe::parse(+axe::r_find(wildcard(wc) >> print), text);
            if (!res.matched)
                ss << wc << " -- not found\n";

            return ss.str();
        };

        const std::string text(R"**(The asterisk in a wildcard matches any character zero or more times.
For example, "comp*" matches anything beginning with "comp" which means
"comp", "complete", and "computer" are all matched.)**");

        gbassert(grep("asterisk*wildcard", text) == R"*(>grep "asterisk*wildcard"
asterisk in a wildcard
)*");
        gbassert(grep("*asterisk*times", text) == R"*(>grep "*asterisk*times"
The asterisk in a wildcard matches any character zero or more times
)*");
        gbassert(grep("\"comp*\"", text) == R"*(>grep ""comp*""
"comp*"
"comp"
"comp"
"complete"
"computer"
)*");
        gbassert(grep("abracadabra", text) == R"*(>grep "abracadabra"
abracadabra -- not found
)*");
    }
}