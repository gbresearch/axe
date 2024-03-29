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

#include "axe_composite_function.h"
#include "axe_terminal_function.h"
#include "axe_extractor_function.h"
#include "axe_predicate.h"
#include "axe_trait.h"

namespace axe
{

    //-------------------------------------------------------------------------
    /// r_number_w_base_t rule matches number with specified base
    //-------------------------------------------------------------------------
    template<unsigned base>
    struct radix;

    template<>
    struct radix<10>
    {
        template<class T>
        static bool match(T value) { return is_num()(value); }

        template<class T, class I>
        static void shift(T& t, I i) { t = t * 10 + *i - '0'; }
    };


    template<class T, unsigned base, bool sign>
    struct r_number_w_base_t;

    template<class T, unsigned base>
    struct r_number_w_base_t<T, base, false> 
    {
        T& number_;
    public:
        explicit r_number_w_base_t(T& number) : number_(number) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            auto matched = false;
            T number{ 0 };

            for(; i1 != i2 && radix<base>::match(*i1); ++i1)
            {
                matched = true;
                radix<base>::shift(number, i1);
            }

            if(matched)
                number_ = number;

            return make_result(matched, i1);
        }
    };

    template<unsigned base>
    struct r_number_w_base_t<void, base, false> 
    {
    public:

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            auto i = i1;
            for(; i != i2 && radix<base>::match(*i); ++i);
            return make_result(i != i1, i);
        }
    };

    template<class T, unsigned base>
    struct r_number_w_base_t<T, base, true> 
    {
        T& number_;
    public:
        explicit r_number_w_base_t(T& number) : number_(number) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            bool sign = false;

            if(i1 != i2)
            {
                if(*i1 == '-')
                {
                    sign = true;
                    ++i1;
                }
                else if(*i1 == '+')
                {
                    ++i1;
                }
            }

            for(; i1 != i2 && is_space()(*i1); ++i1); // skip spaces

            auto result = r_number_w_base_t<T, base, false>(number_)(i1, i2);

            if(result.matched && sign)
                number_ = -number_;

            return result;
        }
    };

    template<unsigned base>
    struct r_number_w_base_t<void, base, true> 
    {
    public:

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            if(i1 != i2 && (*i1 == '-' || *i1 == '+'))
                ++i1;

            for(; i1 != i2 && is_space()(*i1); ++i1); // skip spaces

            auto result = r_number_w_base_t<void, base, false>()(i1, i2);

            return result;
        }
    };

    //-------------------------------------------------------------------------
    // aliases

    template<class T = void>
    using r_udecimal_t = r_number_w_base_t<T, 10, false>;

    template<class T = void>
    using r_decimal_t = r_number_w_base_t<T, 10, true>;


    //-------------------------------------------------------------------------
    /// r_hex_t rule matches unsigned hex number
    //-------------------------------------------------------------------------
    template<class T>
    class r_hex_t 
    {
        T& number_;
    public:
        explicit r_hex_t(T& number) : number_(number) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            T number{ 0 };
            auto start = i1;

            for(; i1 != i2 && is_hex()(*i1); ++i1)
            {
                number = number * 16 + convert(i1);
            }

            auto matched = i1 != start;
            if(matched)
                number_ = number;

            return make_result(matched, i1);
        }
    private:
        template<class Iterator>
        static T convert(Iterator i)
        {
            return *i >= '0' && *i <= '9' ? *i - '0'
                : *i >= 'A' && *i <= 'F' ? *i - 'A' + 10
                : *i - 'a' + 10;
        }
    };

    //-------------------------------------------------------------------------
    /// r_oct_t rule matches unsigned oct number
    //-------------------------------------------------------------------------
    template<class T>
    class r_oct_t 
    {
        T& number_;
    public:
        explicit r_oct_t(T& number) : number_(number) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            T number{ 0 };
            auto start = i1;

            for(; i1 != i2 && is_oct()(*i1); ++i1)
            {
                number = number * 8 + *i1 - '0';
            }

            auto matched = i1 != start;

            if(matched)
                number_ = number;

            return make_result(matched, i1);
        }
    };

    //-------------------------------------------------------------------------
    /// r_binary_t rule matches unsigned binary number
    //-------------------------------------------------------------------------
    template<class T>
    class r_binary_t 
    {
        T& number_;
    public:
        explicit r_binary_t(T& number) : number_(number) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            T number{ 0 };
            auto start = i1;

            for(; i1 != i2 && is_bin()(*i1); ++i1)
            {
                number = number * 2 + *i1 - '0';
            }

            auto matched = i1 != start;
            if(matched)
                number_ = number;

            return make_result(matched, i1);
        }
    };

    //-------------------------------------------------------------------------
    /// r_ufixed_t rule matches unsigned fixed-point number
    //-------------------------------------------------------------------------
    template<class T = void>
    class r_ufixed_t 
    {
        T& number_;
    public:
        explicit r_ufixed_t(T& number) : number_(number) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            unsigned u1 = 0;
            unsigned u2 = 0;
            unsigned length = 0;

            auto result =
                (
                    r_udecimal_t<unsigned>(u1)
                    & ~('.' & ~(r_udecimal_t<unsigned>(u2) >> e_length(length)))
                    | '.' & r_udecimal_t<unsigned>(u2) >> e_length(length)
                    )(i1, i2);

            if(result.matched)
            {
                number_ = u1 + u2 / pow(T(10), T(length));
            }

            return make_result(result.matched, result.position, i1);
        }
    };

    //-------------------------------------------------------------------------
    template<>
    class r_ufixed_t<void> 
    {
    public:
        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            return
                (
                    r_udecimal_t<>()
                    & ~('.' & ~r_udecimal_t<>())
                    | '.' & r_udecimal_t<>()
                    )(i1, i2);
        }
    };

    //-------------------------------------------------------------------------
    /// r_fixed_t rule matches signed fixed-point number
    //-------------------------------------------------------------------------
    template<class T = void>
    class r_fixed_t 
    {
        T&  number_;
    public:

        explicit r_fixed_t(T& number) : number_(number) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            char sign = 0;

            auto result = // optional sign
                (
                    ~(r_char<char>('-') >> sign | '+')
                    & ~r_predstr(is_space())
                    & r_ufixed_t<T>(number_)
                    )(i1, i2);

            if(result.matched && sign == '-')
                number_ *= -1;

            return result;
        }
    };

    //-------------------------------------------------------------------------
    template<>
    class r_fixed_t<void> 
    {
    public:
        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            return
                (
                    ~(r_char<char>('-') | '+')
                    & ~r_predstr(is_space())
                    & r_ufixed_t<>()
                    )(i1, i2);
        }
    };

    //-------------------------------------------------------------------------
    /// r_double_t matches floating point number
    //-------------------------------------------------------------------------
    template<class T = void>
    class r_double_t 
    {
        T& d_;

    public:

        explicit r_double_t(T& d) : d_(d) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            char sign(0);
            unsigned i(0);
            unsigned frac(0);
            int flen(0);
            int e(0);

            auto result =
                (
                    ~(r_lit('-') >> sign | '+')
                    & ~r_predstr(is_space())
                    &
                    (
                        r_udecimal_t<unsigned>(i) & ~('.' & ~r_udecimal_t<unsigned>(frac) >> e_length(flen))
                        | '.' & r_udecimal_t<unsigned>(frac) >> e_length(flen)
                        )
                    & ~(r_any("eE") & r_decimal_t<int>(e))
                    )(i1, i2);

            if(result.matched)
                d_ = (sign == '-' ? -1 : 1) * (T(i) + frac / pow(T(10), T(flen))) * pow(T(10), T(e));

            return result;
        }
    };

    //-------------------------------------------------------------------------
    template<>
    class r_double_t<void> 
    {
    public:
        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            return
                (
                    ~(r_lit('-') | '+')
                    & ~r_predstr(is_space())
                    &
                    (
                        r_udecimal_t<>() & ~('.' & ~r_udecimal_t<>())
                        | '.' & r_udecimal_t<>()
                        )
                    & ~(r_any("eE") & r_decimal_t<>())
                    )(i1, i2);
        }
    };

}
