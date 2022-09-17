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
#include <locale>
#include <algorithm>
#include "axe_trait.h"

namespace axe
{
	namespace detail
	{
		template<class R, class Ret = R>
		using enable_if_rule = std::enable_if_t<AXE_IS_RULE(R), std::decay_t<Ret>>;

		template<class R, class Ret = R>
		using enable_if_extractor = std::enable_if_t<axe::is_extractor_v<std::remove_reference_t<R>>, std::decay_t<Ret>>;

        template<class R, class Ret = R>
        using enable_if_not_extractor = std::enable_if_t<!axe::is_extractor_v<std::remove_reference_t<R>>, std::decay_t<Ret>>;
        
        template <class T> std::decay_t<T> decay_copy(T&& v) { return std::forward<T>(v); }

		// disable single parameter constructors to prevent overloading copy/move ctors
		template<class R, class T>
		using disable_copy = std::enable_if_t<!std::is_same_v<std::decay_t<R>, std::decay_t<T>>>;

		// case conversions
		template<class charT>
		struct toupper
		{
			std::locale loc;
			explicit toupper(std::locale loc) : loc(std::move(loc)) {}
			charT operator()(charT c) const { return std::toupper(c, loc); }
		};

		template<class charT>
		inline std::basic_string<charT> ucase(std::basic_string<charT> str, const std::locale& loc)
		{
			std::transform(str.begin(), str.end(), str.begin(),
				[&](charT c) { return std::toupper(c, loc); });
			return str;
		}

		template<class charT>
		struct tolower
		{
			std::locale loc;
            explicit tolower(std::locale loc) : loc(std::move(loc)) {}
			charT operator()(charT c) const { return std::tolower(c, loc); }

		};

		template<class charT>
		inline std::basic_string<charT> lcase(std::basic_string<charT> str, const std::locale& loc)
		{
			std::transform(str.begin(), str.end(), str.begin(),
				[&](charT c) { return std::tolower(c, loc); });
			return str;
		}
	}
}