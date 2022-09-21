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

#pragma once

#include <exception>
#include <string>
#include <sstream>
#include <array>
#include <iterator>
#include "axe_detail.h"

namespace axe
{
    // the object of "class failure" is thrown when the failure occurs
    // what() function provides a short message
    // message() function provides an additional context
    // a small buffer is used to hold the parsing context

    template<class charT, size_t buflen = 40>
    class failure : public std::exception
    {
        std::string msg;
        std::array<charT, buflen> str; // buffer
    public:
        template<class T, class I>
        failure(T&& msg, I i1, I i2) : msg(std::forward<T>(msg))
        {
            str.fill(0);
            for(size_t i = 0; i1 != i2 && i < str.size() - 1; ++i, ++i1)
                str[i] = *i1;
        }

        template<class T, class = std::enable_if_t<std::is_convertible<T, std::string>::value>>
        explicit failure(T&& msg) : msg(std::forward<T>(msg))
        {
            str.fill(0);
        }

        virtual const char* what() const noexcept { return msg.c_str(); }

        std::basic_string<charT> message() const
        {
            std::basic_stringstream<charT> ss;
            ss << "parser exception";
			if (!msg.empty())
				ss << ":\n   " << msg << "\n";
			else
				ss << ", ";
            ss << "when parsing: \"" << str.data() << "\"";
            return ss.str();
        }
    };

    template<class I>
    inline void throw_failure(std::string msg, I i1, I i2) // [[noreturn]]
    {
        throw failure<typename std::iterator_traits<I>::value_type>(std::move(msg), i1, i2);
    }

    inline void throw_failure(std::string msg) // [[noreturn]]
    {
        throw failure<char>(std::move(msg));
    }

}
