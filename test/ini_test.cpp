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

template<class I>
auto ini(I begin, I end)
{
    using namespace axe::shortcuts;
    std::ostringstream ss;
    
    auto close_section = [&] (const std::string& section)
    {
        if (!section.empty())
        {
            ss << "</" << section << ">\n";
        }
    };

    std::string open_section;
    std::string section_name;

    // section rule, can end with traling spaces
    auto section = *_hs & ('[' & _ident >> section_name & ']')  >> [&]
    {
        close_section(open_section);
        open_section = section_name;
        ss << "<" << open_section << ">\n";
    } & _endl;

    // key name rule, can contain any characters, except '=' and spaces
    std::string key;
    auto key_rule = *_ws & +(_ - '=' - _endl - _hs) >> key;

    // value rules
    std::string value;
    
    // unquoted simple key value
    auto simple_value = *_hs & *(_ - _endl) >> value;
    
    // quoted value may have escaped quote chars
    auto quoted_value = *_hs & '"' & (*(_ - "\\\"" - '"') % +"\\\""_axe) >> value & '"';
    
    // key value can either be quoted or unquoted
    auto value_rule = quoted_value | simple_value;
    
    // rule for property line
    auto prop_line = key_rule & *_hs & '=' & value_rule & _endl >> [&]
    {
        ss << "        <property key=\"" << key << "\" value=\"" << value << "\" />\n";
    };
    
    // rule for comment
    auto comment = *_ws & ';' & *(_ - _endl) & _endl;
    
    // rule for INI file
    auto ini_file = *comment & *(section & *(prop_line | comment)) & *_ws & _z 
        >> [&] { close_section(open_section); }
        | axe::r_fail([&](auto i1, auto i2, auto i3)
    {
        ss << "\nIni file contains errors, indicated by !\n";
        ss << std::string(i1, i2) << "!" << std::string(i2, i3);
    });
    
    // do parsing
    ini_file(begin, end);
    return ss.str();
}

namespace
{
    using namespace gb::yadro::util;

    GB_TEST(axe, test_ini)
    {
        std::string text{ R"**(
    ; This is a test of ini file
    [section1]
    key1 = 5
    key2 = value
    ; this is comment
    [section2]
    key = " quoted value: \"3\" "

)**" };
        
        gbassert(ini(text.begin(), text.end()) == R"*(<section1>
        <property key="key1" value="5" />
        <property key="key2" value="value" />
</section1>
<section2>
        <property key="key" value=" quoted value: \"3\" " />
</section2>
)*");
    }
}