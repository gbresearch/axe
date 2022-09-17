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
#include <map>
#include <vector>
#include <iostream>
#pragma warning(disable:4503)
#include "../include/axe.h"

void parse_roman(const std::string& txt)
{
    using namespace axe;
    using namespace axe::shortcuts;

    unsigned result = 0;
    // thousands
    auto thousands = r_many("M"_axe >> [&result]{ result += 1000; }, 0, 3);
    // hundreds
    auto value100 = "C"_axe >> [&result] { result += 100; };
    auto value400 = "CD"_axe >> [&result] { result += 400; };
    auto value500 = "D"_axe >> [&result] { result += 500; };
    auto value900 = "CM"_axe >> [&result] { result += 900; };
    auto hundreds = value400 | value900 | ~value500 & r_many(value100, 0, 3);
    // tens
    auto value10 = "X"_axe >> [&result] { result += 10; };
    auto value40 = "XL"_axe >> [&result] { result += 40; };
    auto value50 = "L"_axe >> [&result] { result += 50; };
    auto value90 = "XC"_axe >> [&result] { result += 90; };
    auto tens = value90 | value40 | ~value50 & r_many(value10, 0, 3);
    // ones
    auto value1 = "I"_axe >> [&result] { result += 1; };
    auto value4 = "IV"_axe >> [&result] { result += 4; };
    auto value5 = "V"_axe >> [&result] { result += 5; };
    auto value9 = "IX"_axe >> [&result] { result += 9; };
    auto ones = value9 | value4 | ~value5 & r_many(value1, 0, 3);
    // a string of roman numerals separated by spacces
    auto roman = ((thousands & ~hundreds & ~tens & ~ones)
        >> [&result] { std::cout << result << ' '; result = 0; })
        % _s;

    parse(roman, txt);
}

void test_roman()
{
    // test parser
    std::cout << "--------------------------------------------------------test_roman:\n";
    std::string txt("I  MMCCCLVI MMMCXXIII XXIII LVI MMMCDLVII DCCLXXXVI DCCCXCIX MMMDLXVII");
    std::cout << txt << "\n";
    parse_roman(txt);
    std::cout << "\n-----------------------------------------------------------------\n";
}
