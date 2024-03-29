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
#include <tuple>
#include <functional>
#include <iterator>
#include "axe_macro.h"
#include "axe_result.h"

namespace axe
{
    //-----------------------------
    // base classes for rules, extractors, predicates (for backward compatibility)
    //-----------------------------
    class r_base {};
    class e_base {};
    class p_base {};

    //-----------------------------
    // iterator tests
    //-----------------------------
    template<class I>
    constexpr auto is_input_iterator = std::is_base_of_v<std::input_iterator_tag, typename std::iterator_traits<I>::iterator_category>;

    template<class I>
    constexpr auto is_forward_iterator = std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<I>::iterator_category>;

    template<class I>
    constexpr auto is_bidirectional_iterator = std::is_base_of_v<std::bidirectional_iterator_tag, typename std::iterator_traits<I>::iterator_category>;

    template<class I>
    constexpr auto is_random_access_iterator = std::is_base_of_v<std::random_access_iterator_tag, typename std::iterator_traits<I>::iterator_category>;

    //-----------------------------
    // comparison traits
    //-----------------------------
    template<class, class, class = std::void_t<>>
    struct is_eq_comparable_t : std::false_type {};
    template<class U, class V>
    struct is_eq_comparable_t < U, V, std::void_t<decltype(std::declval<U>() == std::declval<V>())>> : std::true_type {};
    template<class U, class V>
    constexpr auto is_eq_comparable = is_eq_comparable_t<U, V>::value;

    //-----------------------------------------------------------------------------------
    // test if function object takes specified arguments
    //-----------------------------------------------------------------------------------
    template< class ...T>
    struct takes_args
    {
        static auto test(...)->std::false_type;
        template<class ...TT>
        static auto test(TT&&...) -> decltype(std::invoke(std::declval<TT>()...), std::true_type{});

        using type = decltype(test(std::declval<T>()...));
        static constexpr bool value = type::value;
    };

    template< class ...T>
    constexpr const bool takes_args_v = takes_args<T...>::value;

    //-----------------------------------------------------------------------------------
    // test if function object takes specified arguments and returns specified value
    //-----------------------------------------------------------------------------------
    template<class T, class Ret, class ...Args> struct is_function_object;

    template<class T, class Ret, class ...Args>
    struct is_function_object<T, Ret(Args...)>
    {
        static auto test(...)->std::false_type;
        template<class TT, class ...AA>
        static auto test(TT&&, AA&&...)
            ->std::enable_if_t<
            std::is_same_v<Ret, decltype(std::invoke(std::declval<TT>(), std::declval<AA>()...))>,
            std::true_type>;

        using type = decltype(test(std::declval<T>(), std::declval<Args>()...));
        static constexpr bool value = type::value;
    };

    template<class T, class Ret, class ...Args>
    constexpr const bool is_function_object_v = is_function_object<T, Ret(Args...)>::value;

    //-------------------------------------------------------------------------
    template<class T, class I>
    using is_rule_object = is_function_object<std::decay_t<T>, result<I>(I, I)>;

    template<class T>
    using is_rule = std::disjunction<
        is_rule_object<T, char*>,
        is_rule_object<T, const char*>,
        is_rule_object<T, signed char*>,
        is_rule_object<T, unsigned char*>,
        is_rule_object<T, const unsigned char*>,
        is_rule_object<T, wchar_t*>,
        is_rule_object<T, const wchar_t*>,
        is_rule_object<T, char16_t*>,
        is_rule_object<T, const char16_t*>,
        is_rule_object<T, char32_t*>,
        is_rule_object<T, const char32_t*>>;

    template<class T>
    constexpr bool is_rule_v = std::is_base_of_v<r_base, std::decay_t<T>> || is_rule<T>::value;

    //-------------------------------------------------------------------------
    template<class T, class I>
    using is_extractor_object = typename std::disjunction<
        is_function_object<std::decay_t<T>, void()>,
        is_function_object<std::decay_t<T>, void(I)>,
        is_function_object<std::decay_t<T>, void(I, I)>,
        is_function_object<std::decay_t<T>, void(I, I, I)>
    >;

    template<class T, class ...I>
    constexpr const bool is_extractor_object_v = is_function_object<std::decay_t<T>, void(I...)>::value;

    template<class T, class I>
    constexpr const bool test_extractor_iterator = is_extractor_object_v<T>
        || is_extractor_object_v<T, I>
        || is_extractor_object_v<T, I, I>
        || is_extractor_object_v<T, I, I>;

    template<class T>
    using is_extractor = std::disjunction<
        is_extractor_object<T, char*>,
        is_extractor_object<T, const char*>,
        is_extractor_object<T, signed char*>,
        is_extractor_object<T, unsigned char*>,
        is_extractor_object<T, const unsigned char*>,
        is_extractor_object<T, wchar_t*>,
        is_extractor_object<T, const wchar_t*>,
        is_extractor_object<T, char16_t*>,
        is_extractor_object<T, const char16_t*>,
        is_extractor_object<T, char32_t*>,
        is_extractor_object<T, const char32_t*>>;

    template<class T>
    constexpr bool is_extractor_v = std::is_base_of_v<e_base, std::decay_t<T>> || is_extractor<T>::value;

    //-------------------------------------------------------------------------
    template<class T, class I>
    constexpr bool is_fail_function_v = std::disjunction<
        is_function_object<std::decay_t<T>, void()>,
        is_function_object<std::decay_t<T>, void(I)>,
        is_function_object<std::decay_t<T>, void(I, I)>,
        is_function_object<std::decay_t<T>, void(I, I, I)>
    >::value;

	//-------------------------------------------------------------------------
    template<class Fn>
    using is_axe_predicate_t = std::disjunction<
        is_function_object<std::decay_t<Fn>, bool(char)>,
        is_function_object<std::decay_t<Fn>, bool(signed char)>,
        is_function_object<std::decay_t<Fn>, bool(unsigned char)>,
        is_function_object<std::decay_t<Fn>, bool(wchar_t)>,
        is_function_object<std::decay_t<Fn>, bool(char16_t)>,
        is_function_object<std::decay_t<Fn>, bool(char32_t)>>;

    template<class Fn>
    constexpr bool is_predicate_v = is_axe_predicate_t<Fn>::value;

	//-------------------------------------------------------------------------
	template<class Fn, class I>
	using is_constraint_t = std::disjunction <
		is_function_object<std::decay_t<Fn>, bool()>,
		is_function_object<std::decay_t<Fn>, bool(I)>,
		is_function_object<std::decay_t<Fn>, bool(I,I)>,
		is_function_object<std::decay_t<Fn>, bool(I, I, I)>>;

	template<class Fn, class I>
	constexpr bool is_constraint_v = is_constraint_t<Fn, I>::value;

	//-------------------------------------------------------------------------
	template<class R>
	class has_name
	{
		struct yes { char c; };
		struct no { yes y[2]; };

		template<class T, class Ret, Ret(T::*)() const> struct const_sfinae {};

		template<class T>
		static yes test(T* t, const_sfinae<T, std::string, &T::name>* = 0);
		template<class T>
		static yes test(T* t, const_sfinae<T, const std::string&, &T::name>* = 0);
		template<class T>
		static yes test(T* t, const_sfinae<T, const char*, &T::name>* = 0);

		template<class T>
		static no test(...);
	public:
		static const bool value = sizeof(yes) == sizeof(test<R>(0));
	};

}
