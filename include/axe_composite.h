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

#include <utility>
#include <functional>
#include <stddef.h>
#include <typeinfo>
#include <tuple>
#include <variant>
#include <optional>
#include "axe_result.h"
#include "axe_trait.h"
#include "axe_iterator.h"
#include "axe_exception.h"
#include "axe_detail.h"

namespace axe
{
    namespace detail
    {
        template<class R, class... Rs>
        class r_binary_fn_t
        {
            static_assert(sizeof...(Rs) != 0);
            std::tuple<R, Rs...> rs_;

        public:
            template<class T1, class T2, class...Ts>
            r_binary_fn_t(T1&& r1, T2&& r2, Ts&&... rs)
                : rs_(std::forward<T1>(r1), std::forward<T2>(r2), std::forward<Ts>(rs)...)
            {}

            decltype(auto) get() const & { return rs_; }
            decltype(auto) get() && { return std::move(rs_); }
        };

        // if rule doesn't define an extracting operator(), then provide a generic one
        template<class R, class I>
        auto parse_tree_invoke(R&& r, it_pair<I> it)
        {
            if constexpr (is_extracting_rule_v<I, R>)
            {
                return std::invoke(std::forward<R>(r), it);
            }
            else
            {
                auto res = std::invoke(std::forward<R>(r), it.begin(), it.end());
                return result(it_pair(it.begin(), res.position), res.matched, res.position);
            }
        }
        
        // result type of parse_tree invocation
        template<class R, class I>
        using parse_tree_result_t = decltype(parse_tree_invoke(std::declval<R>(), it_pair<I>{}));

        // return data type of extracting rule
        template<class R, class I>
        using parse_tree_data_t = typename parse_tree_result_t<R, I>::data_t;

        // helpers for r_or_t
        template<size_t N, class A, class...T>
        auto make_variant_helper(std::variant<T...>&& v) -> std::variant<A, T...>
        {
            assert(v.index() < sizeof...(T));
            if (v.index() == N) return std::variant<A, T...>(std::in_place_index<N + 1>, std::move(std::get<N>(v)));
            if constexpr (N + 1 < sizeof...(T))
            {
                return make_variant_helper<N + 1, A, T...>(std::move(v));
            }
            else
            {
                return {};
            }
        }

        template<class A, class...T>
        auto make_variant(std::variant<T...>&& v) { return make_variant_helper<0, A>(std::move(v)); }


        // result type of parse_tree invocation
        template<class R, class I>
        using parse_tree_result_t = decltype(parse_tree_invoke(std::declval<R>(), it_pair<I>{}));

        // return data type of extracting rule
        template<class R, class I>
        using parse_tree_data_t = typename parse_tree_result_t<R,I>::data_t;

    }

    //-----------------------------------------------------------------------------
    /// class r_and_t defines AND operation
    //-----------------------------------------------------------------------------

    template<class R, class... Rs>
    class r_and_t final : public detail::r_binary_fn_t<R, Rs...>
    {
        using base = detail::r_binary_fn_t<R, Rs...>;

        template<class Iterator, class Iterator2, class Rule, class...Rules>
        static auto match(Iterator i1, Iterator2 i2, Rule&& r, Rules&&... rs)-> result<Iterator>
        {
            auto res = std::invoke(std::forward<Rule>(r), i1, i2);
            if constexpr (sizeof...(Rules) != 0)
            {
                if (res.matched)
                    res = match(res.position, i2, std::forward<Rules>(rs)...);
            }
            return res;
        }

        template<class Iterator, class Rule, class...Rules>
        static auto match_tree(it_pair<Iterator> itp, Rule&& r, Rules&&... rs)->result<Iterator, 
            std::tuple< detail::parse_tree_data_t<Rule, Iterator>, detail::parse_tree_data_t<Rules, Iterator>...>>
        {
            using data_t = std::tuple< detail::parse_tree_data_t<Rule, Iterator>, detail::parse_tree_data_t<Rules, Iterator>...>;

            auto res = detail::parse_tree_invoke(std::forward<Rule>(r), itp);
            data_t data;
            std::get<0>(data) = res.data;

            if constexpr (sizeof...(Rules) != 0)
            {
                if (res.matched)
                {
                    auto res1 = match_tree(it_pair(res.position, itp.end()), std::forward<Rules>(rs)...);
                    return result(std::apply([&](auto&&...d)
                    {
                        return std::make_tuple(std::move(res.data), std::forward<decltype(d)>(d)...);
                    }, res1.data), res1.matched, res1.position);
                }
            }
            return result(data, res.matched, res.position);//rslt;
        }

    public:
        using base::base;
        using base::get;

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            static_assert(is_forward_iterator<Iterator>);
            return std::apply([&](auto&&...r) { return match(i1, i2, std::forward<decltype(r)>(r)...); }, get());
        }

        template<class Iterator>
        auto operator() (it_pair<Iterator> itp) const -> result<Iterator,
            std::tuple< detail::parse_tree_data_t<R, Iterator>, detail::parse_tree_data_t<Rs, Iterator>...>>
        {
            static_assert(is_forward_iterator<Iterator>);
            return std::apply([&](auto&&...r) { return match_tree(itp, std::forward<decltype(r)>(r)...); }, get());
        }
        static const char* name() { return "and"; }
    };

    //-----------------------------------------------------------------------------
    /// class r_or_t defines OR operation
    //-----------------------------------------------------------------------------

    template<class R, class... Rs>
    class r_or_t final : public detail::r_binary_fn_t<R, Rs...>
    {
        using base = detail::r_binary_fn_t<R, Rs...>;

        template<class Iterator, class Iterator2, class Rule, class...Rules>
        static auto match(Iterator i1, Iterator2 i2, Rule&& r, Rules&&... rs)
        {
            auto res = std::invoke(std::forward<Rule>(r), i1, i2);
            if constexpr (sizeof...(Rules) != 0)
            {
                if (!res.matched)
                {
                    auto pos = res.position;
                    res = match(i1, i2, std::forward<Rules>(rs)...);
                    if (!res.matched && std::distance(i1, pos) > std::distance(i1, res.position))
                        res.position = pos; // if failed return the longest match
                }
            }
            return res;
        }

        template<class Iterator, class Rule, class...Rules>
        static auto match_tree(it_pair<Iterator> itp, Rule&& r, Rules&&... rs)->
            result<Iterator, std::variant<detail::parse_tree_data_t<Rule, Iterator>, detail::parse_tree_data_t<Rules, Iterator>...>>
        {
            using data_t = std::variant<detail::parse_tree_data_t<Rule, Iterator>, detail::parse_tree_data_t<Rules, Iterator>...>;
            auto res = detail::parse_tree_invoke(std::forward<Rule>(r), itp);
            
            result<Iterator, data_t> rslt(
                data_t( std::in_place_index<0>, res.data ),
                res.matched, res.position
            );

            if constexpr (sizeof...(Rules) != 0)
            {
                if (!res.matched)
                {
                    auto res1 = match_tree(itp, std::forward<Rules>(rs)...);
                    if (res1.matched)
                    {
                        return result<Iterator, data_t>(
                            detail::make_variant<detail::parse_tree_data_t<Rule, Iterator>>(std::move(res1.data)),
                            res1.matched, res1.position
                        );
                    }
                    else if (std::distance(itp.begin(), res1.position) > std::distance(itp.begin(), res.position))
                    {   // failed, choose the longest match
                        rslt.position = res1.position;
                    }
                }
            }
            return rslt;
        }

    public:
        using base::base;
        using base::get;

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            static_assert(is_forward_iterator<Iterator>);
            return std::apply([&](auto&&...r) { return match(i1, i2, std::forward<decltype(r)>(r)...); }, get());
        }
        
        template<class Iterator>
        auto operator() (it_pair<Iterator> itp) const->
            result<Iterator, std::variant<detail::parse_tree_data_t<R, Iterator>, detail::parse_tree_data_t<Rs, Iterator>...>>
        {
            static_assert(is_forward_iterator<Iterator>);
            return std::apply([&](auto&&...r) { return match_tree(itp, std::forward<decltype(r)>(r)...); }, get());
        }

        static const char* name() { return "or"; }
    };

    //-----------------------------------------------------------------------------
    /// class r_xor_t defines permutation: A ^ B === A & ~B | B & ~A
    //-----------------------------------------------------------------------------
    template<class R1, class R2>
    class r_xor_t final
    {
        R1 r1_;
        R2 r2_;

    public:
        template<class T1, class T2>
        r_xor_t(T1&& r1, T2&& r2) : r1_(std::forward<T1>(r1)), r2_(std::forward<T2>(r2)) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            auto&& rslt = r1_(i1, i2);
            if(rslt.matched)
            {
                auto&& rslt2 = r2_(rslt.position, i2);
                return make_result(true, rslt2.matched ? rslt2.position : rslt.position);
            }
            else
            {
                rslt = r2_(i1, i2);
                if(rslt.matched)
                {
                    auto&& rslt2 = r1_(rslt.position, i2);
                    return make_result(true, rslt2.matched ? rslt2.position : rslt.position);
                }
            }

            return make_result(false, i1);
        }
    };

    //-----------------------------------------------------------------------------
    /// class r_not_t defines NOT (negation) operation
    //-----------------------------------------------------------------------------
    template<class R>
    class r_not_t final
    {
        R r_;

    public:
		template<class T, class = detail::disable_copy<r_not_t<R>, T>>
        explicit r_not_t(T&& r) : r_(std::forward<T>(r)) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            auto&& i = r_(i1, i2);
            return make_result(!i.matched, i1, i.position);
        }
    };

    //-----------------------------------------------------------------------------
    /// class r_select_t defines select rule (r1 & r2 | !r1 & r3)
    //-----------------------------------------------------------------------------
    template<class R1, class R2, class R3>
    class r_select_t final
    {
        R1 r1_;
        R2 r2_;
        R3 r3_;
    public:
        template<class T1, class T2, class T3>
        r_select_t(T1&& r1, T2&& r2, T3&& r3) : r1_(std::forward<T1>(r1)),
            r2_(std::forward<T2>(r2)), r3_(std::forward<T3>(r3))
        {
        }

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            auto&& match = r1_(i1, i2);
            return match.matched ? r2_(match.position, i2) : r3_(i1, i2);
        }
    };

    //-----------------------------------------------------------------------------
    /// class r_many_t defines a sequence of rules separated by separator rule
    //-----------------------------------------------------------------------------
    template<class R, class S>
    class r_many_t final
    {
        R r_;
        S separator_;
        const size_t min_occurrence_;
        const size_t max_occurrence_;

    public:
        template<class TR, class TS>
        r_many_t(TR&& r, TS&& separator, size_t min_occurrence, size_t max_occurrence)
            : r_(std::forward<TR>(r)), separator_(std::forward<TS>(separator)),
            min_occurrence_(min_occurrence), max_occurrence_(max_occurrence)
        {
        }

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2)  const
        {
            auto i_match = r_(i1, i2);

            if(!i_match.matched)
                return make_result(!min_occurrence_, i1, i_match.position);

            size_t count = 1;
            auto match = i_match;

            while(match.matched && count < max_occurrence_)
            {
                match = separator_(match.position, i2);
                if(match.matched)
                    match = r_(match.position, i2);
                if(match.matched)
                {
                    i_match = match;
                    ++count;
                }
            }

            return make_result(count >= min_occurrence_, i_match.position, match.position);
        }

        template<class Iterator>
        auto operator() (it_pair<Iterator> itp)  const
            ->result<Iterator, std::vector < detail::parse_tree_data_t<R, Iterator>>>
        {
            using r_data_t = detail::parse_tree_data_t<R, Iterator>;
            std::vector<r_data_t> data;
            
            size_t count = 0;
            auto rule_match = true;
            auto sep_match = true;
            auto i = itp.begin();
            auto last_pos = i;

            while (rule_match && sep_match && count < max_occurrence_)
            {
                auto rule_res = detail::parse_tree_invoke(r_, it_pair(i, itp.end()));
                rule_match = rule_res.matched;
                last_pos = rule_res.position;

                if (rule_match)
                {
                    ++count;
                    data.push_back(std::move(rule_res.data));

                    // separators don't extract data
                    auto sep_res = std::invoke(separator_, rule_res.position, itp.end());
                    sep_match = sep_res.matched;
                    if(sep_match)
                        i = sep_res.position;
                    else
                        i = rule_res.position;
                }
            }

            auto matched = count >= min_occurrence_;
            return result(std::move(data), matched, matched ? i : last_pos);
        }
    };

    //-----------------------------------------------------------------------------
    /// class r_opt_t defines optional operation (always a match)
    //-----------------------------------------------------------------------------
    template<class R>
    class r_opt_t final
    {
        R r_;
    public:
        template<class T, class = detail::disable_copy<r_opt_t<R>, T>>
        explicit r_opt_t(T&& r) : r_(std::forward<T>(r)) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2)  const
        {
            auto&& i = r_(i1, i2);
            return make_result(true, i.matched ? i.position : i1);
        }

        template<class Iterator>
        auto operator() (it_pair<Iterator> itp)  const
            ->result<Iterator, std::optional < detail::parse_tree_data_t<R, Iterator>>>
        {
            using data_t = detail::parse_tree_data_t<R, Iterator>;
            auto rule_res = detail::parse_tree_invoke(r_, itp);
            if(rule_res.matched)
                return result(std::optional<data_t>{rule_res.data},
                true, rule_res.position);
            else
                return result(std::optional<data_t>{}, true, itp.begin());
        }
    };

    //-----------------------------------------------------------------------------
    /// reference wrapper (lvalues held by reference, rvalues moved)
    //-----------------------------------------------------------------------------
    template<class R>
    class r_ref_t final
    {
        R r_;
    public:
        explicit r_ref_t(R&& r) : r_(std::forward<R>(r)) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2)  const
        {
            return r_(i1, i2);
        }
    };

    //-----------------------------------------------------------------------------
    /// rule to find specified rule (skip input elements until specified rule matched)
    //-----------------------------------------------------------------------------
    template<class R>
    class r_find_t final
    {
        R r_;
    public:
        template<class T, class = detail::disable_copy<r_find_t<R>,T>>
        r_find_t(T&& r) : r_(std::forward<T>(r)) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2)  const
        {
            auto&& match = make_result(false, i1);

            for(; i1 != i2 && !match.matched; ++i1)
            {
                match = r_(i1, i2);
            }

            return match;
        }
        
        template<class Iterator>
        auto operator() (it_pair<Iterator> itp)  const -> detail::parse_tree_result_t<R, Iterator>
        {
            auto res = detail::parse_tree_invoke(r_, itp);

            for (; !itp.empty() && !res.matched; itp.next())
            {
                res = detail::parse_tree_invoke(r_, itp);
            }

            return res;
        }
    };

    //-----------------------------------------------------------------------------
    /// r_fail_t matches the first rule and if failed calls specified function
    //-----------------------------------------------------------------------------
    template<class R, class F>
    class r_fail_t final
    {
        R r_; // rule to match
        F f_; // function to call on fail
    public:
        template<class TR, class TF>
        r_fail_t(TR&& r, TF&& f) : r_(std::forward<TR>(r)), f_(std::forward<TF>(f)) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2)  const
        {
            auto&& match = r_(i1, i2);
            if(!match.matched)
            {
                static_assert(is_fail_function_v<F, Iterator>);

                if constexpr(is_function_object_v<F, void>)
                    std::invoke(f_);
                else if constexpr(is_function_object_v<F, void, Iterator>)
                    std::invoke(f_, match.position);
                else if constexpr(is_function_object_v<F, void, Iterator, Iterator>)
                    std::invoke(f_, match.position, i2);
                else if constexpr (is_function_object_v<F, void, Iterator, Iterator, Iterator>)
                    std::invoke(f_, i1, match.position, i2);
            }

            return match;
        }
    };

    //-----------------------------------------------------------------------------
    /// r_fail_wrapper_t is used to wrap fail function
    //-----------------------------------------------------------------------------
    template<class F>
    class r_fail_wrapper_t final
    {
        F f_;
    public:
        template<class T, class = detail::disable_copy<r_fail_wrapper_t<F>, T>>
        r_fail_wrapper_t(T&& f) : f_(std::forward<T>(f)) {}
        
        const F& operator()() const& { return f_; }
        F&& operator()() && { return std::move(f_); }
        F get() const { return f_; }
    };

    //-----------------------------------------------------------------------------
    // throw_fail_t action function used in r_fail(string) to throw exception
    //-----------------------------------------------------------------------------
    class throw_fail_t final
    {
        std::string str;
    public:
        throw_fail_t(std::string str) : str(std::move(str)) {}

        template<class Iterator, class Iterator2>
        void operator() (Iterator i1, Iterator2 i2)  const
        {
            throw_failure(std::move(str), i1, i2);
        }
    };

    //-----------------------------------------------------------------------------
    /// r_test_t matches the specified rule, but always returns the initial iterator
    //-----------------------------------------------------------------------------
    template<class R>
    class r_test_t final
    {
        R r_;
    public:
        template<class T, class = detail::disable_copy<r_test_t<R>, T>>
        r_test_t(T&& r) : r_(std::forward<T>(r)) {}

        template<class Iterator, class Iterator2>
        result<Iterator> operator() (Iterator i1, Iterator2 i2) const
        {
            return make_result(r_(i1, i2).matched, i1);
        }
    };

    //-----------------------------------------------------------------------------
    /// r_rule is a polymorphic rule, used primarily for defining recursive rules
    //-----------------------------------------------------------------------------
    template<class I>
    class r_rule final
    {
        std::function<result<I>(I, I)> fun_;
    public:
        r_rule() = default;

        template<class Fun, class = detail::disable_copy<r_rule<I>, Fun>>
        r_rule(Fun&& fun) : fun_(std::forward<Fun>(fun)) {}

        template<class Fun, class = detail::disable_copy<r_rule<I>, Fun>>
        r_rule& operator= (Fun&& fun) { fun_ = std::forward<Fun>(fun); return *this; }

        explicit operator bool() const { return (bool)fun_; }

        template<class Iterator, class Iterator2>
        result<Iterator> operator()(Iterator i1, Iterator2 i2) const
        {
            static_assert(std::is_convertible<Iterator, I>::value);
            if (fun_)
                return fun_(i1, i2);
            else // always match an empty rule
                return result(true, i1);
        }
    };

    //-----------------------------------------------------------------------------
    // unordered AND class (match in any order)
    //-----------------------------------------------------------------------------
    template<class R1, class R2>
    class r_unordered_and_t final
    {
        R1 r1_;
        R2 r2_;
    public:
        template<class T1, class T2>
        r_unordered_and_t(T1&& r1, T2&& r2) : r1_(std::forward<T1>(r1)), r2_(std::forward<T2>(r2)) {}

        template<class Iterator>
        result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            auto match = r1_(i1, i2);
			if (match.matched)
			{
				match = r2_(match.position, i2);
			}
			else
            {
                match = r2_(i1, i2);
                if(match.matched)
                    match = r1_(match.position, i2);
            }
            return match;
        }
    };

    //-----------------------------------------------------------------------------
    // r_seq_or_t implements R1 & R2 | R1 | R2;
    //-----------------------------------------------------------------------------
    template<class R1, class R2>
    class r_seq_or_t final
    {
        R1 r1_;
        R2 r2_;
    public:
        template<class T1, class T2>
        r_seq_or_t(T1&& r1, T2&& r2) : r1_(std::forward<T1>(r1)), r2_(std::forward<T2>(r2)) {}

        template<class Iterator>
        result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            auto match = r1_(i1, i2);
            if(match.matched)
            {
                auto match1 = r2_(match.position, i2);
                return make_result(true, match1.matched ? match1.position : match.position);
            }
            return r2_(i1, i2);
        }
    };

    //-----------------------------------------------------------------------------
    // r_atomic_t matches all or nothing implements R1 & (R2 | r_fail());
    //-----------------------------------------------------------------------------
    template<class R1, class R2>
    class r_atomic_t final
    {
        R1 r1_;
        R2 r2_;
    public:
        template<class T1, class T2>
        r_atomic_t(T1&& r1, T2&& r2) : r1_(std::forward<T1>(r1)), r2_(std::forward<T2>(r2)) {}

        template<class Iterator>
        result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            auto match = r1_(i1, i2);
            if(match.matched)
            {   // if r1_ matched r2_ must match too
                match = r2_(match.position, i2);
                if(!match.matched)
                    throw_failure(std::string("R1 > R2 rule failed with \n   R1: ") + get_name(r1_)
                        + "\n   R2: " + get_name(r2_),
                        match.position, i2);

                return make_result(true, match.position);
            }
            return match;
        }
    };

    //-----------------------------------------------------------------------------
    // skip rule
    //-----------------------------------------------------------------------------
    template<class R, class F>
    class r_skip_t final
    {
        R r_;
        F f_;
    public:
        template<class TR, class TF>
        r_skip_t(TR&& r, TF&& f) : r_(std::forward<TR>(r)), f_(std::forward<TF>(f)) {}

        template<class I, class I2>
        result<I> operator() (I i1, I2 i2) const
        {
            auto rslt = r_(skip_iterator(i1, i2, f_), skip_iterator(i2, i2, f_));
            return result(rslt.matched, rslt.position.get());
        }
    };

    //-----------------------------------------------------------------------------
    // convert rule
    //-----------------------------------------------------------------------------
    template<class R, class F>
    class r_convert_t final
    {
        R r_;
        F f_;
    public:
        template<class TR, class TF>
        r_convert_t(TR&& r, TF&& f) : r_(std::forward<TR>(r)), f_(std::forward<TF>(f)) {}

        template<class I>
        result<I> operator() (I i1, I i2) const
        {
            convert_iterator<I, F> begin(i1, f_);
            convert_iterator<I, F> end(i2, f_);
            auto rslt = r_(begin, end);
            return axe::make_result(rslt.matched, rslt.position.get());
        }
    };

    //-----------------------------------------------------------------------------
    // buffered rule
    //-----------------------------------------------------------------------------
    template<class R>
    class r_buffered_t final
    {
        R r_;
    public:
        template<class T, class = detail::disable_copy<r_buffered_t<R>, T>>
        r_buffered_t(T&& r) : r_(std::forward<T>(r)) {}

        template<class I>
        result<I> operator() (I i1, I i2) const
        {
            input_buffer<I> buf(i1, i2);
            auto begin = buf.begin();
            auto end = buf.end();
            auto rslt = r_(begin, end);
            // buffered rule must not fail, because there is no way to roll back input iterator
            if(!rslt.matched)
            {
                // copy to buffer 40 chars for reporting purposes
                const decltype(std::distance(begin, end)) report_length = 40;
                auto i = rslt.position;
                std::advance(i, std::min(report_length, std::distance(i, end)));
                throw_failure("buffered rule failed", rslt.position, i);
            }
            return result(rslt.matched, rslt.position.get());
        }
    };

    //-----------------------------------------------------------------------------
    // named rule
    //-----------------------------------------------------------------------------
    template<class R>
    class r_named_t final
    {
        R r_;
        const char* name_;
    public:
        template<class T>
        r_named_t(T&& r, const char* name) : r_(std::forward<T>(r)), name_(name) {}

        template<class I>
        result<I> operator() (I i1, I i2) const { return r_(i1, i2); }

        const char* name() const { return name_; }
    };

	//-----------------------------------------------------------------------------
	// constrained rule
	// after matching the rule constraint is checked
	// if constraint is not satisfied the r_constrained rule fails
	//-----------------------------------------------------------------------------
	template<class R, class Fn>
	class r_constrained final
	{
		R r_;
		Fn constraint_;

		template<class I>
		auto test_constraint(I i1, I i2, I i3) const
		{
			static_assert(is_constraint_v<Fn, I>);

			if constexpr (is_function_object_v<std::decay_t<Fn>, bool>)
				return std::invoke(constraint_);
			else if constexpr (is_function_object_v<Fn, bool, I>)
				return std::invoke(constraint_, i1);
			else if constexpr (is_function_object_v<Fn, bool, I, I>)
				return std::invoke(constraint_, i1, i2);
			else if constexpr (is_function_object_v<Fn, bool, I, I, I>)
				return std::invoke(constraint_, i1, i2, i3);
		}

	public:
		template<class RR, class FF>
		r_constrained(RR&& r, FF&& constraint) 
			: r_(std::forward<RR>(r)), constraint_(std::forward<FF>(constraint))
		{}

		template<class I>
		result<I> operator() (I i1, I i2) const 
		{
			static_assert(is_input_iterator<I>);

			auto rslt = r_(i1, i2);

			if (rslt.matched)
			{
				rslt.matched = test_constraint(i1, rslt.position, i2);
			}

			return rslt; 
		}

		template<class I>
		auto operator() (it_pair<I> itp)  const
			->result<I, detail::parse_tree_data_t<R, I>>
		{
			static_assert(is_input_iterator<I>);

			auto rslt = r_(itp);
			if (rslt.matched)
			{
				rslt.matched = test_constraint(itp.begin(), rslt.position, itp.end());
			}

			return rslt;
		}

		const char* name() const { return "r_constrained"; }
	};

	template<class RR, class FF>
	r_constrained(RR&& r, FF&& constraint)
		->r_constrained<std::remove_reference_t<RR>, std::remove_reference_t<FF>>;
}