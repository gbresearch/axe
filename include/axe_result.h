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
#include <type_traits>
#include <utility>
#include <iterator>
#include <functional>

namespace axe
{
    //-------------------------------------------------------------------------
    /// result class holds parsed data, whether the rule was matched, and end iterator position
    //-------------------------------------------------------------------------
    template<class Iterator, class Data = void>
    struct result
    {
        Data                data;
        bool                matched;
        Iterator            position;
        using data_t =      Data;
        using iterator_t =  Iterator;

        explicit operator bool() const { return matched; }

        result() = default;

        template<class D, class = decltype(Data{ std::declval<D>() })>
        result(D&& d, bool matched, Iterator position) : data(std::forward<D>(d)),
            matched(matched), position(position)
        {}
    };

    template<class Iterator, class D>
    result(D&&, bool, Iterator)->result<Iterator, std::decay_t<D>>;

    //-------------------------------------------------------------------------
    /// result class holds the result of rule matching and iterator position
    //-------------------------------------------------------------------------
    template<class Iterator>
    struct result<Iterator, void>
    {
        bool                matched;
        Iterator            position;
        using data_t =      void;
        using iterator_t =  Iterator;

        explicit operator bool() const { return matched; }

        result(bool matched, Iterator position) : matched(matched), position(position) {}
    };

    template<class Iterator>
    result(bool, Iterator)->result<Iterator>;

    //-------------------------------------------------------------------------
    /// make_result function constructs result class
    //-------------------------------------------------------------------------
    template<class Iterator>
    inline auto make_result(bool b, Iterator position)
    {
        return result(b, position);
    }

    //-------------------------------------------------------------------------
    /// make_result function constructs result class depending on the value
    //-------------------------------------------------------------------------
    template<class Iterator>
    inline auto make_result(bool b, Iterator success_position, Iterator fail_position)
    {
        return result(b, b ? success_position : fail_position);
    }

}
