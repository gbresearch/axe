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
#include <sstream>
#include <yadro/util/gbtest.h>
#pragma warning(disable:4503)
#include "../include/axe.h"

auto find_paths(const std::wstring& path_string)
{
    using namespace axe;
    using namespace axe::shortcuts;
    std::wostringstream ss;

    // spaces are allowed in quoted paths only

    // illegal path characters
    auto illegal = r_any("/?<>\\:*|\",");

    // end of line characters
    auto endl = r_any("\n\r");

    // define path characters
    auto path_chars = _ - illegal - _hs - endl;

    // windows path can start with a server name or letter
    auto start_server = "\\\\" & +path_chars - '\\';
    auto start_drive = r_alpha() & ':';
    auto simple_path = (start_server | start_drive) & *('\\' & +path_chars);
    auto quoted_path = '"' & (start_server | start_drive) &
        *('\\' & +(_hs | path_chars)) & '"';

    // path can be either simple or quoted
    auto path = simple_path | quoted_path;

    // rule to extract all paths
    std::vector<std::wstring> paths;
    auto extract_paths = *(_ - path) % (path >> e_push_back(paths)) & _z;

    // perform extraction
    extract_paths(std::begin(path_string), std::end(path_string));

    // print extracted paths
    std::for_each(paths.begin(), paths.end(),
        [&](const std::wstring& s)
    {
        ss << s << L' ';
    });

    return ss.str();
}

namespace
{
    using namespace gb::yadro::util;

    GB_TEST(axe, test_winpath)
    {
        gbassert(find_paths(LR"*(not-a-path, c:\a\b\c, \\server\a\b\c, "c:\quoted path")*") == LR"*(c:\a\b\c \\server\a\b\c "c:\quoted path" )*");
    }
}