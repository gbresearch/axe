//-----------------------------------------------------------------------------
//  Copyright (C) 2011-2018, GB Research, LLC (www.gbresearch.com)
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

#include "axe_result.h"
#include "axe_composite_function.h"
#include "axe_terminal_function.h"
#include "axe_extractor_function.h"
#include "axe_predicate_function.h"
#include "axe_numeric_function.h"

namespace axe
{

    //-------------------------------------------------------------------------
    /// e_plus_t, e_minus_t, e_mult_t, e_div_t - common extractors
    /// operators used +=, -=, *=, /=
    //-------------------------------------------------------------------------

    template<class T> struct r_expression;

    //-------------------------------------------------------------------------
    /// r_group_t - rule for syntactical group (paranthesised expression)
    //-------------------------------------------------------------------------
    template<class T>
    struct r_group_t
    {
        T& value_;
        explicit r_group_t(T& value) : value_(value) {}

        template<class It>
        result<It> operator() (It i1, It i2) const
        {
            return ('(' & r_expression<T>(value_) & ')')(i1, i2);
        }
    };

    //-------------------------------------------------------------------------
    /// r_factor_t - rule for factor
    //-------------------------------------------------------------------------
    template<class T>
    struct r_factor_t
    {
        T& value_;
        explicit r_factor_t(T& value) : value_(value) {}

        template<class It>
        result<It> operator() (It i1, It i2) const
        {
            if constexpr(std::is_floating_point_v<T>)
                return (r_double(value_) | r_group_t<T>(value_))(i1, i2);
            else
                return (r_decimal(value_) | r_group_t<T>(value_))(i1, i2);
        }
    };

    //-------------------------------------------------------------------------
    /// r_term_t - rule for term
    //-------------------------------------------------------------------------
    template<class T>
    struct r_term_t
    {
        T& value_;
        explicit r_term_t(T& value) : value_(value) {}

        template<class It>
        result<It> operator() (It i1, It i2) const
        {
            T t;
            return (r_factor_t<T>(value_)
                & *( '*' & r_factor_t<T>(t) >> [&] { value_ *= t; }
                | '/' & r_factor_t<T>(t) >> [&] { value_ /= t; }
                ))(i1, i2);
        }
    };

    //-------------------------------------------------------------------------
    /// r_expression - rule for expression
    //-------------------------------------------------------------------------
    template<class T>
    struct r_expression
    {
        T& value_;
        explicit r_expression(T& value) : value_(value) {}

        template<class It>
        result<It> operator() (It i1, It i2) const
        {
			T t{};
            return
                (r_term_t<T>(value_)
                    & *('+' & r_term_t<T>(t) >> [&] { value_ += t; } 
                    | '-' & r_term_t<T>(t) >> [&] { value_ -= t; }
                    ))(i1, i2);
        }
    };

    template<class T>
    r_expression(T&)->r_expression<T>;

    template<class T, class C, class Traits, class Alloc>
    inline auto parse_expression(const std::basic_string<C, Traits, Alloc>& str, T def_value)
    {
        auto result{ def_value };
        return r_skip(r_expression(result), r_pred(is_wspace()))
            (str.begin(), str.end()).matched ? result : def_value;
    }
}