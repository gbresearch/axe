# Review of the AXE 2.1 Library Reference against the current code

This report lists the parts of [`AXE-Reference-v2.1.pdf`](AXE-Reference-v2.1.pdf) (41 pages, 2019) that no longer
described the code in `include/` at commit `771ddc1`, with a suggested fix for each. The code references link to that
commit.

## Resolution

- **Code.** The defects of section S were fixed in the code, with tests in `test/rules_test.cpp`, together with four
  more found while fixing them (S12-S15). B1 (`A && B`) and I2 (`r_named` in a parse tree) were also fixed in the code,
  so the guide's description of them became true. N1 (the iterator passed to a one-argument extractor) was kept in the
  code and corrected in the guide, because existing parsers may rely on it.
- **Guide.** Every other finding was applied to [`AXE-Reference.md`](AXE-Reference.md), which also gained the
  features of section R and a *Changes after v.2.1* section. All its examples compile without warnings with GCC 14
  (C++17, C++20) and Clang 20 (C++20), and the examples that show their output print it.
- **Open.** The version number and copyright line on the title page (A4) are unchanged and need the maintainer's
  decision. The test suite still doesn't exercise `r_skip`, `r_buffered`, `r_convert` and the case conversion rules
  (recommendation 4).

## How the review was done

- Every statement in the guide was checked against the headers at commit `771ddc1` (the current `master`).
- Every code example was extracted from `AXE-Reference.md`, compiled with GCC 13.3 (`-std=c++17` and `-std=c++20`)
  and Clang 18.1 (`-std=c++20`) against `include/`, and run. Its output was compared with the output printed in the guide.
- Each suspected discrepancy was confirmed with a small test program. The **Evidence** lines quote what those programs printed.
- Every replacement snippet suggested below was compiled with the same three configurations and run.

Kinds of findings:

| Label | Meaning |
|---|---|
| **Wrong** | The guide says something the code doesn't do. Following it gives wrong results. |
| **Broken example** | An example doesn't compile, doesn't terminate, or doesn't print what the guide says. |
| **Outdated** | True in 2019, no longer true. |
| **Incomplete** | Correct, but leaves out behavior a user needs. |
| **Missing** | A feature of the library the guide doesn't mention. |
| **Typo** | Editorial. |

## Summary

| | Count | Most important |
|---|---|---|
| Wrong | 11 | One-argument extractors get the *end* of the match (N1); `r_array` and `r_sequence` are described as something else (E5, E6); `A > B` and `r_named` don't produce the documented parse tree (I1, I2); `A && B` isn't `A & B \| B & A` (B1) |
| Broken example | 11 | The Windows path example never terminates (P4); the custom rule lambda isn't a rule (L1); the custom extractor class doesn't compile when used (O1); the `r_rule` example is a syntax error (H1) |
| Outdated | 5 | C++17 and Visual C++ 2017 requirements, the GCC n3599 literal (A1, A2, D1); version and copyright (A4); the gbresearch.com links, which couldn't be checked (M3) |
| Incomplete, Missing | 25 | the parse-tree rows and constraint signatures (B2, I3), extraction target types (N3), `get_as` overloads (J2); `r_depth_limit`, the UTF-8 rules and the `failure` exception are not in the guide at all (section R) |
| Typo | 8 | `axe;r_oct()`, `axe::alnumstr()`, `axe_util.h`, `r_expression_t` |

Section S lists the defects in the code found while checking the guide: eleven during the review, four more (S12-S15) while fixing them. They are not documentation errors, but several
of them decide what the guide should say (for example S2, the `r_bool` end-of-input behavior).

Nine of the eleven example programs in the *Examples* chapter compile. Five of them print what the guide shows, two
(replacement, word count) have no output in the guide to compare with, the CSV example prints different years than the
guide shows (P2), and the Windows path example never terminates (P4). The telephone and INI examples don't compile as
printed (P1, P3).

---

## A. Title page, Introduction, Changes in v.2

### A1. Compiler requirements — **Outdated**
**Guide** (Introduction, p. 3; *Changes in v.2*, "compiler" row, p. 4): "The library uses standard C++17 … It's been tested
with Visual C++ 2017 and expected to work with any other reasonably compliant C++17 compiler."

**Code**: [`axe_macro.h:33-39`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_macro.h#L33-L39) still requires C++17 (or MSVC 19.14). C++20 adds the
template string literal and `char8_t` input ([`axe_shortcut.h:86-104`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_shortcut.h#L86-L104)).
[`README.md`](../README.md#requirements) lists the compilers the code is tested with today.

**Suggested text**: "AXE needs C++17; C++20 is needed for `char8_t` input and for the compile-time string literals
(`"abc"_axe`). The library uses only standard C++. It is tested with Visual Studio 2026 (`/std:c++latest`), GCC 14 and
Clang 20 (`-std=c++23`); each header also compiles on its own with GCC and Clang at C++17, C++20 and C++23." Better still,
refer to the *Requirements* section of the README so the two can't drift apart.

### A2. Footnote 2, GCC n3599 literal operator template — **Outdated**
**Guide** (p. 3, footnote 2): "GCC literal operator template implementation (n3599) used as an optimization".

**Code**: no compiler extension is used any more. With C++20 every compiler uses the standard class-type template
parameter (P0732, `detail::string_literal`); with C++17 `"abc"_axe` is `r_str`.

**Suggested fix**: delete the footnote (see D1 for the new operator description).

### A3. Header name `axe_util.h` — **Typo**
**Guide** (*Changes in v.2*, "Parse tree functions", p. 4): "convenience functions in axe_util.h".

**Code**: the header is [`axe_utility.h`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_utility.h).

### A4. Version number and copyright line — **Outdated** (maintainer's decision)
The title says *AXE 2.1* and every page says *Copyright © 2011-2019 GB Research, LLC.* The headers say
*Copyright (C) 2011-2022, Gene Bushuyev* and the Makefile *2011-2026*. Since 2.1 the library gained `r_depth_limit`,
the UTF-8 rules, C++20 literals for all character types, the const-`operator()` checks and the C++23 port, and there
is no version macro in the code. Suggest choosing a new version number (e.g. 2.2), updating the copyright line in the
front matter of `AXE-Reference.md` (it is printed on every page), and adding a *Changes in v.2.2* section built from
section R below.

---

## B. Syntax Rules › AXE operator rules (p. 5)

### B1. `A && B` is not `A & B | B & A` — **Wrong**
**Guide**: "Match rule A and B in any order (same as `A & B | B & A`)".

**Code**: `r_unordered_and_t` ([`axe_composite.h:646`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L646)) tries `B` then `A` only when `A`
fails at the start. When `A` matches but `B` doesn't follow, it fails without trying `B & A`.

**Evidence**: with `A = r_char('a')`, `B = r_str("ab")` and input `"aba"`, `A && B` doesn't match; `A & B | B & A` does.

**Suggested fix**: either change the code to fall back to `B & A` (then the guide is right), or change the text to:
"Match A followed by B or, if A doesn't match, B followed by A. Unlike `A & B | B & A`, B followed by A is not tried
when A matches but B doesn't follow it."

### B2. `A/C`: the constraint's signature isn't given — **Incomplete**
**Code**: `r_constrained` ([`axe_composite.h:824-881`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L824-L881)) accepts a constraint
callable as `bool()`, `bool(I first)`, `bool(I first, I last)` or `bool(I first, I last, I end)`, where `[first, last)`
is the range matched by A and `end` is the end of the input.

**Suggested addition**: those four signatures, and "C is tested only when A matched; if C returns false, the whole rule
fails."

### B3. `A – B` uses an en dash — **Typo** (fixed in the Markdown conversion)
Copying `A – B` from the PDF doesn't compile. The PDF has typographic quotes in code throughout (`“str”_axe`, `‘!’`);
`AXE-Reference.md` uses plain ASCII in all code.

### B4. `~A` "same as `r_many(A, 0, 1)`" — **Incomplete**
They match the same input, but in a parse tree `~A` produces `std::optional<DA>` and `r_many(A, 0, 1)` produces
`std::vector<DA>`. Suggest adding "(in a parse tree, `std::optional` instead of `std::vector`)".

## C. AXE Shortcuts (p. 6)

| Row | Guide | Fix |
|---|---|---|
| `_o` | `axe;r_oct()` | `axe::r_oct()` — **Typo** |
| `_w` | `axe::alnumstr()` | `axe::r_alnumstr()` — **Typo** ([`axe_shortcut.h:81`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_shortcut.h#L81)) |
| `_ws` | White space: `" \t\n\r"` | White space: `axe::r_pred(axe::is_wspace())`, one of `' '`, `'\t'`, `'\n'`, `'\r'` — **Incomplete** |
| `_hs`, `_ident` | `r_any(" \t")`, `r_ident()` | add the `axe::` prefix used in the other rows — **Typo** |

## D. AXE operator"" (p. 6)

### D1. `"str"_axe` — **Outdated**
**Guide**: "Creates `axe::r_str("str")` rule. For GCC (implementing n3599) this operator creates `r_strlit<'s','t','r'>`".

**Code** ([`axe_shortcut.h:86-112`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_shortcut.h#L86-L112)): with C++20 or later, on every compiler,
`"str"_axe` is `r_strlit<'s','t','r'>` and `""_axe` is `r_empty`; it works with `char`, `wchar_t`, `char8_t`, `char16_t`
and `char32_t` literals, and characters after an embedded `\0` are matched too. With C++17 it is `r_str` and works with
`char` and `wchar_t` literals.

**Suggested text**: the *Literal shortcuts* section of [`README.md`](../README.md#literal-shortcuts-_axe-_any-_regex)
already has the right wording.

### D2. Character literals and wide strings — **Missing**
`'c'_axe` (`char` or `wchar_t`) creates `r_char('c')` ([`axe_shortcut.h:114-123`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_shortcut.h#L114-L123)).
`_any` and `_regex` also accept `wchar_t` strings. Add a `'c'_axe` row.

---

## E. AXE functions and class constructors (pp. 7-11)

### E1. `r_expression`, `parse_expression` — **Incomplete**
**Guide**: "`parse_expression` … returns either expression value if evaluation succeeded, or `def_value` if it failed.
The white-space characters are skipped."

**Code** ([`axe_expression.h:127-133`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_expression.h#L127-L133)): the result is the value of the longest
*prefix* that is an expression; trailing text is ignored. White space is skipped everywhere, including inside numbers,
because the rule runs under `r_skip`.

**Evidence**: `parse_expression("1+2xyz"s, -1)` returns `3`; `parse_expression("1 2"s, -1)` returns `12`.

**Suggested addition**: "The expression may use `+`, `-`, `*`, `/` and parentheses. `parse_expression` doesn't require the
whole string to be an expression: it returns the value of the longest leading expression, and `def_value` only if the
string doesn't start with one. Because white space is skipped between all characters, `"1 2"` is read as 12."

### E2. `r_fail(F)`: which iterators the function receives — **Incomplete**
**Guide** (p. 7): "Function receives upto three iterators: beginning of range, failed position, end of range."

**Code** ([`axe_composite.h:539-556`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L539-L556)): `void(I)` receives the failed position;
`void(I, I)` the failed position and the end of the input; `void(I, I, I)` the position where the failed rule started,
the failed position and the end.
The *Semantic Actions* table (p. 21) describes this correctly. Suggest copying that wording here.

### E3. `r_fail(string = "")` — **Wrong**
**Code** ([`axe_composite_function.h:100-109`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite_function.h#L100-L109)): the overloads are
`r_fail(std::string)` and `r_fail(const char* str = "r_fail")`, so `r_fail()` throws with the message `"r_fail"`.

**Suggested text**: "`r_fail(str = "r_fail")`: when the preceding rule fails, `axe::failure<charT>` is thrown;
`what()` returns `str`, and `message()` adds up to 39 characters of the input at the failed position."

### E4. `r_bool(b)`: the example fails when the number is at the end of the input — **Broken example**
**Code** ([`axe_terminal.h:74-75`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L74-L75)): `r_bool` never matches at the end of the input.

**Evidence**: the guide's rule `_int >> number & r_bool([&] { return number > 0 && number < 1000; })` doesn't match
`"123"`, but matches `"123;"`. The `r_constrained` example on p. 8 matches `"123"` and rejects `"1234"`.

**Suggested fix**: decide S2 first. If the check stays, add "`r_bool` doesn't match at the end of the input" and
replace the example with the `r_constrained` one. Also document that `b` may be a function taking no arguments, one
iterator (the current position) or two (the current position and the end), and see S1: a lambda without captures is
currently treated as the constant `true`.

### E5. `r_array(a)` — **Wrong**
**Guide**: "Matches a static array of rules (`std::array`). Same as `r_and<T…>`."

**Code** ([`axe_terminal.h:391-419`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L391-L419)): `r_array(std::array<T, N>& a)` reads `N`
binary values of type `T` from the input into `a` (`sizeof(T)` bytes each, like `r_var`); it needs forward iterators
over bytes.

**Evidence**: `parse(r_array(a), "xyzw"s)` with `std::array<char, 3> a` matches and leaves `a == {'x','y','z'}`.

**Suggested text**: "`r_array(std::array<T, N>& a)`: reads `N` values of the standard layout type `T` from binary input
into `a`, `sizeof(T)` bytes each. Fails if the input is shorter."

### E6. `r_sequence(c, min_occurrence = 0, max_occurrence = -1)` — **Wrong**
**Guide**: "Matches a sequence of rules specified number of times".

**Code** ([`axe_terminal.h:428-461`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L428-L461)): reads up to `max_occurrence` binary values of
`C::value_type` from the input and appends them to the container `c` with `push_back`; matches if `c` then holds at least
`min_occurrence` values. `c` is cleared when the rule is constructed, not on each parse, so parsing twice with the same
rule object appends to the values of the first parse.

**Evidence**: `parse(r_sequence(v, 2, 3), "pqrs"s)` with `std::vector<char> v` matches and leaves `v == {'p','q','r'}`.

**Suggested text**: the above, including the note about clearing.

### E7. `r_ident()` "(`r_alpha() & r_alnumstr(0)`)" — **Wrong**
`r_alnumstr(0)` matches exactly zero characters, so the expression in the guide matches a single letter.

**Code** ([`axe_terminal.h:473-489`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L473-L489)): a letter or `_` followed by any number of
letters, digits and `_`.

**Evidence**: on `"ab1 "`, `r_alpha() & r_alnumstr(0)` consumes 1 character; `r_ident()` and `r_alpha() & *r_alnum()` consume 3.

**Suggested text**: "Matches an identifier (`r_alpha() & *r_alnum()`)".

### E8. `r_advance(size)`: the example doesn't compile and the idiom doesn't work — **Broken example**
**Guide** (p. 9): `*(r_any() - (R >> e_length(s)) & r_advance(s); // matches R once`
- The parentheses are unbalanced.
- `r_advance` stores a copy of its argument ([`axe_terminal.h:513`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L513)), so `r_advance(s)`
  advances by the value `s` had when the rule was built (usually 0), not by the length `e_length(s)` stored during the
  parse. `r_advance(std::ref(s))` reads it at parse time.
- This is also why the Windows path example never terminates (P4).

**Suggested text** (verified):

```cpp
size_t s = 0;
*(r_any() - R) & R;                                        // R is matched twice: by r_any() - R, then by R
*(r_any() - (R >> e_length(s))) & r_advance(std::ref(s));  // R is matched once
```

and "`size` is copied when the rule is created; pass `std::ref(n)` to use the value `n` has when the rule is evaluated."

### E9. `r_range` — **Incomplete**
There is also `r_range(const Container& c)`, matching the elements of `c` ([`axe_terminal.h:549`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L549)).

### E10. Numeric rules — **Incomplete**
- `r_udecimal()`, `r_decimal()`, `r_ufixed()`, `r_fixed()`, `r_double()` without an argument match without storing a value
  (the shortcuts `_uint`, `_int`, `_double` use them).
- `r_hex(t)`, `r_oct(t)` and `r_binary(t)` match unsigned hexadecimal, octal and binary numbers and store the value in `t`
  ([`axe_numeric_function.h:53-68`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_numeric_function.h#L53-L68)). Note that `r_hex()` and `r_oct()` without
  an argument are the single-character rules.
- The signed rules accept spaces between the sign and the digits: `r_decimal(n)` reads `"- 42"` as -42.

### E11. Binary-digit rules and `r_anystr(n)` — **Missing**
`r_binary()`, `r_binstr()`, `r_binstr(occurrence)`, `r_binstr(min_occurrence, max_occurrence)` match `'0'` and `'1'`
([`axe_terminal_function.h:279-303`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal_function.h#L279-L303)). `r_anystr(occurrence)` matches any
`occurrence` characters ([`axe_terminal_function.h:366`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal_function.h#L366)). Add rows next to the
octal and hexadecimal rules.

### E12. Character class rows — **Typo**
"octodecimal" should be "octal" (four rows); "Matched octodecimal character" should be "Matches an octal character".
"Printable" means the ASCII characters `' '` to `'~'` ([`axe_predicate.h:105`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_predicate.h#L105)), and
`is_alpha` is ASCII letters and `_` only. Saying so avoids surprises with non-ASCII input.

### E13. `r_regex`: "should usually start with ^" — **Incomplete**
`std::regex_search` returns the leftmost match, and the rule rejects a match that doesn't start at the current position,
so `^` isn't needed for correctness. It only stops the search from scanning the rest of the input when there is no
match at the current position. `r_regex` needs bidirectional iterators.

**Evidence**: `r_regex("[a-z]+")` matches `"abc123"` without `^`; `r_regex("[0-9]+")` doesn't.

**Suggested text**: "The rule matches when the regular expression matches at the current position. Starting the
expression with `^` makes failing matches faster, because the search then doesn't scan the rest of the input.
Needs bidirectional iterators."

### E14. `r_str(str)` with a `const charT*` keeps the pointer — **Incomplete**
`r_str(const CharT*)` stores the pointer, not a copy ([`axe_terminal.h:199-202`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L199-L202));
`r_str(std::basic_string)` copies the string. Suggest: "the character array must outlive the rule, e.g.
`r_str(s.c_str())` dangles once `s` changes; pass the `std::string` instead."

### E15. `r_lit(a)` — **Incomplete**
`r_lit` accepts `char`, `wchar_t` (as `r_char`), `const char*`, `const wchar_t*` (as `r_str`) and other arithmetic
types (as `r_bin`), not `std::string`. Note that `char16_t` and `char32_t` count as "other arithmetic types" and are
matched byte by byte. The example's `r_lit(1.23)` is affected by S3: it doesn't match its own bytes in a `std::string`.

---

## F. AXE Iterator transformation rules (p. 12)

### F1. `r_buffered`: exception type — **Wrong**
**Guide**: "an `axe::r_failure` exception is thrown".

**Code** ([`axe_composite.h:794`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L794)): `axe::failure<charT>` with the message
`"buffered rule failed"`. There is no `r_failure`.

### F2. `r_skip`, `r_convert`, `r_buffered` change the iterator type — **Incomplete**
Their sub-rule receives `skip_iterator<I, F>`, `convert_iterator<I, F>` or `input_buffer<I>::iterator`, so an `r_rule`
used inside must be declared with that type, and `r_skip` skips between every pair of characters, not only between tokens
(`r_skip(r_decimal(n), _ws)` reads `1 2` as 12). The README section
[*`r_rule` inside rules that change the iterator type*](../README.md#r_rule-inside-rules-that-change-the-iterator-type)
explains both and can be moved into the guide.

### F3. `r_ucase`, `r_lcase` don't convert the string — **Incomplete**
The input is converted and compared with the string as given, so the string must already be upper (lower) case.

**Evidence**: `_int & r_ucase("L")` matches `"5l"`; `_int & r_ucase("l")` doesn't.

Suggest adding "The string itself isn't converted: write it in upper case (lower case for `r_lcase`)."

### F4. `r_convert(R, F)` — **Incomplete**
`F` is applied to each input element (not to the iterator), and its result must be convertible to the iterator's
`value_type`. Suggest: "Creates a rule that passes R an iterator whose elements are the input elements converted by F."

## G. AXE Predicate Functions (p. 13)

### G1. `is_bin()` — **Missing**
`is_bin` returns true for `'0'` and `'1'` ([`axe_predicate.h:93`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_predicate.h#L93)). It may also help to
say that `is_alpha`, `is_num`, … are types, and `is_alpha()` creates an object of that type.

## H. AXE polymorphic rule (p. 13)

### H1. The example is a syntax error — **Broken example**
**Guide**: `r = "[" & _int | a "]";`

**Suggested replacement** (verified: `a & _z` matches `"abc[abc[12]]"`):

```cpp
axe::r_rule<Iterator> r;
auto a = "abc"_axe & std::ref(r);
r = '[' & (_int | a) & ']';
```

### H2. Default `r_rule`, iterator type — **Incomplete**
A default-constructed `r_rule` matches empty input and converts to `false`
([`axe_composite.h:618-640`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L618-L640)). `r_rule<I>` can only be called with iterators
convertible to `I` (a `static_assert` says so); see F2 for `r_skip`, `r_convert` and `r_buffered`.

---

## I. AXE Parse Tree bindings (p. 14)

### I1. `A > B > …` — **Wrong**
**Guide**: `std::tuple<DA, DB, …>`.

**Code**: `r_atomic_t` ([`axe_composite.h:701`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L701)) has no parse-tree `operator()`, so the
data is `axe::it_pair<I>`.

**Evidence**: `decltype(parse_tree(_d > _a, s).data)` is `it_pair<std::string::iterator>`.

**Suggested fix**: remove the row (it falls under "all other rules"), or add parse-tree support to `r_atomic_t` and keep it.

### I2. `r_named(A)` — **Wrong**
**Guide**: "`r_find<A>`, `r_named<A>`: DA -- the same type as for rule A".

**Code**: `r_named_t` ([`axe_composite.h:804`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L804)) has no parse-tree `operator()`, so its data
is `it_pair<I>`. `r_find(A)` does produce `DA`.

**Evidence**: `parse_tree(r_named(_d & _a, "x"), s).data` is `it_pair`; `parse_tree(r_find(_d & _a), s).data` is a tuple.

**Suggested fix**: "`r_find(A)`, `A/C`, `r_depth_limit(A, n)`: DA". Also write the rules as calls, `r_find(A)`, not `r_find<A>`.

### I3. Rows to add — **Incomplete**
- `A/C` and `r_depth_limit(A, n)` produce `DA`.
- `A >> E` produces `it_pair<I>`, and the extractor still runs. With `AXE_ALLOW_PARSE_TREE_EXTRACTION` defined,
  `A >> E` produces `DA` and passes it to `E` ([`axe_extractor.h:85-100`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_extractor.h#L85-L100)).
- `A && B`, `A || B`, `A ^ B`, `!A`, `r_test(A)`, `r_select(A, B, C)` and the transformation rules produce `it_pair<I>`.
  Listing them avoids readers expecting tuples, as in I1.

## J. AXE Utility functions (pp. 15-16)

### J1. `parse_expression` isn't in `axe_utility.h` — **Wrong**
It is in [`axe_expression.h:127`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_expression.h#L127). Move the row, or change the introduction to
"defined in the axe namespace".

### J2. `get_as` overloads — **Incomplete**
The table lists only `get_as<D>(const Data&)`, but the `r_constrained` example on p. 8 already uses another overload.
All of them ([`axe_utility.h:214-259`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_utility.h#L214-L259)):
`get_as<D>(I first, I last)`, `get_as<D>(const it_pair<I>&)`, `get_as<D>(const Data&)`,
`get_as<D>(const result<I, T>&)` and `get_as<D>(const result<I, T>&, Fn fn)`, which converts `fn(res.data)`.
The conversion uses the in-place extractors (see N3), so for example `get_as<int>` throws `std::invalid_argument` when the
text isn't a number, and returns an indeterminate value for an empty match (S11).

### J3. `write_xml` output format — **Incomplete**
Each nesting level is indented by one space; `<vector size="n">`, `<tuple size="n">`, `<variant index="i">` and
`<optional matched="true|false">` carry those attributes; an empty vector is `<vector size="0"/>`. See S5 for the
closing tag of `<optional>`.

---

## K. Creating custom predicates (p. 17)

### K1. `axe::is_predicate<custom>::value` doesn't exist — **Wrong**
**Code** ([`axe_trait.h:179-189`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_trait.h#L179-L189)): the trait is `axe::is_predicate_v<custom>`. It
is true when `custom` returns `bool` for an argument of type `char`, `signed char`, `unsigned char`, `wchar_t`,
`char16_t` or `char32_t`.

**Evidence**: `static_assert(axe::is_predicate<is_num>::value)` fails to compile: "'is_predicate' is not a member of 'axe'".

### K2. Lambda example — **Broken example**
`auto is_num = [](auto c) { return c >= '0' && c <= '9'; }` lacks the `;`.

## L. Creating custom rules (pp. 17-18)

### L1. The lambda example isn't a rule — **Broken example**
**Guide**: `return axe::result(i1 != i2 && *i1 == '\n', i1 + 1, i1);`
With three arguments, class template argument deduction picks `result(Data, bool matched, Iterator position)`, so the
lambda returns `result<I, bool>` (with `matched` set from the iterator `i1 + 1`), not `result<I>`. `axe::is_rule_v` is
false, and the lambda doesn't compile for `std::string` iterators. `i1 + 1` is also past the end when `i1 == i2`.

**Suggested replacement** (verified):

```cpp
auto custom = [](auto i1, auto i2)
{
    bool matched = i1 != i2 && *i1 == '\n';
    return axe::result(matched, matched ? std::next(i1) : i1);
};
```

### L2. `const operator()` is now enforced — **Incomplete**
The text says custom rules define a const `operator()`. `&`, `-` and `|` now check it with a `static_assert`,
"every operand must be a rule with a const operator()" ([`axe_composite.h:177`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L177)), and a
rule without one isn't accepted by the operators. Worth saying explicitly, together with the error message.

## M. Recursive rules (pp. 18-20)

### M1. `r_expression_t` — **Typo**
The forward-declared rule is `r_expression`, not `r_expression_t` ([`axe_expression.h:46`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_expression.h#L46)).

### M2. The `operator()` in the excerpt lacks `const` — **Incomplete**
The excerpt shows `result<It> operator() (It i1, It i2)`; the code has `const`, and without it the rules wouldn't be
accepted (L2). Add `const` to the four excerpts.

### M3. Links to gbresearch.com — **Outdated?**
The links to `www.gbresearch.com/axe/dox/…` (here, in *Creating custom rules*, and *Doxygenated sources*) couldn't be
checked from the review environment, and the repository has no Doxygen configuration. Suggest linking to the headers on
GitHub instead, e.g. `include/axe_expression.h`.

---

## N. Semantic Actions › AXE in-place extractors (pp. 21-22)

### N1. A one-argument extractor receives the end of the match — **Wrong**
**Guide**: "If the extractor accepts one argument then the iterator pointing to the beginning of matched rule will be passed".

**Code** ([`axe_extractor.h:74-76`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_extractor.h#L74-L76)): it receives `i.position`, the position after
the match.

**Evidence**: `parse(r_alphastr() >> [&](auto i) -> void { … }, "abc123"s)` receives offset 3, not 0.

**Suggested text**: "If the extractor accepts one argument, the iterator pointing past the end of the matched range is
passed." (Or change the code, if the beginning was intended; existing users may rely on the current behavior.)

### N2. `e_push_back(c)` — **Incomplete**
Each matched range is converted to `C::value_type` with the same conversions as `A >> t` (N3) and appended with
`emplace_back`, so `e_push_back` works for e.g. `std::vector<int>` as well as `std::vector<std::string>`.

### N3. `A >> t`, extracting into a variable — **Missing**
The guide uses `r_ident() >> name` but never says which types `t` can have. `operator>>` with an lvalue that isn't an
extractor uses `e_value_t<T>` ([`axe_extractor.h:109-469`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_extractor.h#L109-L469)):

| Type of `t` | Value stored |
|---|---|
| `std::basic_string` | the matched text |
| `std::basic_string_view` | a view of the matched text; compiles only with pointer iterators such as `const char*` (S10) |
| `char`, `signed char`, `unsigned char`, `wchar_t`; `std::byte` (only from `std::byte` input) | the first matched element |
| `short` … `unsigned long long` | `std::stoi`, `std::stol`, … of the text (they throw on text that isn't a number) |
| `float`, `double`, `long double` | `std::stof`, `std::stod`, `std::stold` of the text |
| `bool` | whether the match is non-empty |
| `std::basic_ostream` | writes the matched text to the stream, e.g. `rule >> std::cout` |
| any other type | read from the text with `operator>>` |

### N4. `r_fail(F)` example — **Broken example**
`auto on_fail = [] { … }` lacks the `;` after the closing brace.

### N5. Lambdas are instantiated with pointer iterators — **Incomplete**
The traits test candidate extractors (and rules) with `char*`, `const char*`, `wchar_t*`, …, so a lambda with a deduced
return type is instantiated with those types. A body that compiles only for the real iterator type, e.g. one that
subtracts `text.begin()`, is a hard error inside the traits. The *Creating custom rules* section explains this for rules;
add the same note for extractors, with the remedy of writing `-> void`.

## O. Creating custom semantic actions (p. 22)

### O1. The custom extractor class doesn't compile when used — **Broken example**
Its `operator()` isn't `const`. The `static_assert(axe::is_extractor_v<custom>)` in the guide passes, but `r_extractor_t`
calls the extractor from its own `const operator()` ([`axe_extractor.h:62-81`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_extractor.h#L62-L81)), so
`rule >> custom(n)` fails with "no matching function for call to invoke(const custom&, …)".

**Suggested fix** (verified): declare it `void operator()(Iterator i1, Iterator i2) const`; the counter is a reference
member, so it can still be incremented. Add "The `operator()` of an extractor must be `const`."

---

## P. Examples (pp. 24-40)

| Example | Compiles | Output as in the guide |
|---|---|---|
| Zip code parser | yes | yes |
| Telephone number parser | **no** (P1) | the guide shows no output |
| CSV parser | yes | **no** (P2) |
| INI file parser | **no** (P3) | yes, after the fix |
| Command line parser | yes | yes |
| Windows path parser | yes | **never terminates** (P4) |
| Roman numerals | yes | yes |
| JSON parser | yes | yes, but see P5 |
| Replacement parser | yes | the guide shows no output |
| Word counting program | yes | the guide shows no output |
| Wildcard parser | yes | yes |

### P1. Telephone number parser: `slog` isn't declared — **Broken example**
The other examples print to `std::cout`. Replace `slog` with `std::cout` (verified). The guide could also show the
output, which demonstrates the constraint: the fax numbers 650-617-3120 and 408-278-7444 are excluded.

```text
(650)329-2100
650-329-2258
408-278-7400
```

(followed by the same three numbers again after "Using parse_tree function:"). Note that this constrained version is not
part of the test suite; `test/zip_test.cpp` tests the version without the constraint.

### P2. CSV parser: the output doesn't match the input — **Broken example**
The program parses `2017,Ford,…` and `2018 , Toyota,…`, but the guide shows `<2010>` and `<2011>`. Change the input to
2010 and 2011, as in `test/cvs_test.cpp`, or the output to 2017 and 2018.

### P3. INI file parser: `template<class I>` is missing — **Broken example**
`void ini(I begin, I end)` doesn't compile; add `template<class I>` before it. The guide's output shows the property lines
indented by five spaces; the program writes a tab.

### P4. Windows path parser never terminates — **Broken example**
`*(*(r_any() - (path >> …  >> e_length(length))) & r_advance(length))` uses `r_advance(length)` with the value `length`
had when the rule was built, 0 (E8). Once the inner rule stops, `r_advance(0)` matches without consuming input, and
`*(…)` repeats it forever (see also S7).

**Evidence**: the program runs until killed, for every input including an empty string. With `r_advance(std::ref(length))`
it terminates when the text ends right after a path, but still loops on text without paths.

**Suggested fix**: use the extraction rule from `test/winpath_test.cpp` (verified on paths, on text without paths and on
empty input):

```cpp
auto extract_paths = *(_ - path) % (path >> e_push_back(paths)) & _z;
```

and drop `length` and `r_advance`. The test also adds `,` to the illegal characters, so that a path in a comma separated
list doesn't include the comma.

### P5. JSON parser rejects empty arrays and objects — **Incomplete**
The grammar requires at least one value in an array and one record in an object, and an object at the top level.

**Evidence**: `{"a": []}` and `{"a": {}}` fail with "parsing failed at place pointed by !".

**Suggested fix**: either the minimal change (verified: accepts `{}`, `{"a": {}, "b": [1, [2, []]]}`, rejects `{"a": [1,]}`):

```cpp
auto json_array = *_ws & '[' & *_ws & (']'_axe | (*_ws & std::ref(json_value) & *_ws) % ',' & ']');
auto json_object = *_ws & '{' & ~(json_record % ',') & *_ws & '}';
```

or the RFC 8259 grammar from `test/jason_test.cpp`, which also validates UTF-8 and escapes and limits the nesting depth
with `r_depth_limit`, and so demonstrates two features the guide lacks (R1, R2).

### P6. Smaller issues — **Typo**
- Replacement parser: "returns a pair" — it returns a `std::tuple`; "replace the the portions" repeats "the".
- Roman numerals: the program prints a space after the last number. `test/roman_numerals.cpp` separates the numerals with
  `% +_s`, which also accepts several spaces.
- *Examples* introduction: the examples also need `<map>`, `<set>`, `<numeric>`, `<fstream>`, `<stdexcept>` and
  `<functional>`, not only `<axe.h>`.

## Q. Doxygenated sources (p. 41)
See M3. If the Doxygen pages are no longer published, suggest replacing the section with a link to the repository and to
the headers, which have documentation comments.

---

## R. Features missing from the guide

| # | Feature | Where it is documented today |
|---|---|---|
| R1 | `r_depth_limit(rule, max_depth)` and `axe::depth_limit_exceeded<I>` (`position()`, `max_depth()`): bounding recursion so deep input can't overflow the stack | `axe_depth.h`, [README](../README.md#limiting-recursion-depth-r_depth_limit) |
| R2 | `r_utf8()`, `r_utf8str()`: validating UTF-8 | `axe_utf8.h`, [README](../README.md#validating-utf-8-r_utf8-r_utf8str) |
| R3 | `axe::failure<charT>`: the exception thrown by `r_fail(str)`, `A > B` and `r_buffered`; `what()` and `message()` | `axe_exception.h` only |
| R4 | `r_rule` with `r_skip`, `r_convert`, `r_buffered` (F2) | [README](../README.md#r_rule-inside-rules-that-change-the-iterator-type) |
| R5 | C++20 literals for all character types, `'c'_axe`, `""_axe` (D1, D2) | [README](../README.md#literal-shortcuts-_axe-_any-_regex) |
| R6 | `is_bin`, `r_binary`, `r_binstr`, `r_anystr(n)`, numeric `r_hex(t)`, `r_oct(t)`, `r_binary(t)`, `r_range(container)` (E9-E11, G1) | headers only |
| R7 | Extraction target types (N3), all `get_as` overloads (J2), `it_pair` and `make_it_pair` | headers only |
| R8 | `AXE_ALLOW_PARSE_TREE_EXTRACTION` (I3) | headers only |
| R9 | Iterator requirements: `&`, `\|`, `-` and the UTF-8 rules need forward iterators (checked by `static_assert`), `r_regex` bidirectional; input iterators only through `r_buffered` | headers only |
| R10 | Building and testing (Makefile, Visual Studio, Yadro) | [README](../README.md#building-and-running-the-tests) |

Moving the README sections R1, R2, R4 and R5 into the guide (and keeping a short pointer in the README) would make the
guide complete again.

## S. Defects in the code found during the review

These are not documentation errors. Each needs either a code change or a sentence in the guide. All were reproduced with
GCC 13.3 and Clang 18.1.

| # | Defect | Location | Suggested change |
|---|---|---|---|
| S1 | `r_bool([] { return false; })` always matches: a lambda without captures converts to a function pointer and then to `bool`, so it is taken for a constant `true` | [`axe_terminal.h:69-78`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L69-L78) | test the function-object cases before the convertible-to-`bool` case |
| S2 | `r_bool` never matches at the end of the input (E4) | [`axe_terminal.h:74`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L74) | remove the `i1 == i2` check (`r_bool` doesn't read input), or document it |
| S3 | `r_bin` compares the bytes as `unsigned char` with `char` input, so any byte of 0x80 or more never matches on platforms where `char` is signed: `r_bin(1.0)` doesn't match its own bytes in a `std::string`, but does in a `std::vector<unsigned char>` | [`axe_terminal.h:158`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L158) | compare with `static_cast<unsigned char>(*i1)` |
| S4 | `parse_tree` of a constrained rule whose rule has no parse-tree `operator()` doesn't compile, e.g. `parse_tree(_ident / [](auto, auto) { return true; }, s)` | [`axe_composite.h:871`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L871) | call `detail::parse_tree_invoke(r_, itp)` instead of `r_(itp)` |
| S5 | `write_xml` closes `<optional matched="true">` with `/optional>` | [`axe_utility.h:151`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_utility.h#L151) | `</optional>` |
| S6 | `r_many(literal, literal, …)` constructs the separator with the type of the first literal: `r_many('a', ", ")` doesn't compile | [`axe_terminal_function.h:123`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal_function.h#L123) | `detail::r_lit_traits_t<R2>(r2)` |
| S7 | `*A`, `+A` and `A % B` don't stop when an iteration consumes no input, so they never finish when A (and B) can match empty input: `parse(*(*_d), "abc"s)` doesn't terminate | [`axe_composite.h:385`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L385) | stop when the position doesn't advance, or warn in the guide |
| S8 | The comment on `operator^` says "but not both (`r1 & !r2 \| !r1 & r2`)"; the implementation and the guide match permutations | [`axe_operator.h:138`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_operator.h#L138) | fix the comment |
| S9 | The parse-tree `operator()` of `r_find` evaluates the rule twice at the starting position | [`axe_composite.h:512-520`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L512-L520) | advance before re-evaluating |
| S10 | Extracting into a `std::string_view` doesn't compile with `std::string` iterators: it constructs the view from an iterator and a length | [`axe_extractor.h:156`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_extractor.h#L156) | construct it from `&*i1` (contiguous input) or restrict it to pointers in the guide |
| S11 | `get_as<D>(first, last)` default-initializes `D d;`, so for an arithmetic `D` and an empty range it returns an indeterminate value | [`axe_utility.h:216`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_utility.h#L216) | value-initialize: `D d{};` |
| S12 | `parse_tree` of `A % B` consumed a trailing separator (`_d % ','` on `"1,2,x"` consumed 4 characters, `parse` 3), and evaluated the separator once more after the last allowed occurrence | [`axe_composite.h:400-436`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_composite.h#L400-L436) | end the match after the last occurrence; stop before the separator at the maximum |
| S13 | `r_many('x', 2)` took `2` for a binary separator instead of the minimum count, and then didn't compile (S6) | [`axe_terminal_function.h:113-124`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal_function.h#L113-L124) | exclude integer separators, as the rule-and-literal overload does |
| S14 | `r_ident` and `r_sequence` raise GCC 14 `-Wdangling-reference` with `const char*` input | [`axe_terminal.h:479`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L479) | store the result by value |
| S15 | `r_advance` with a `size_t` offset, as `e_length` stores it, raises `-Wsign-compare` | [`axe_terminal.h:526-527`](https://github.com/gbresearch/axe/blob/771ddc1ba8c0afa940529a1de8e6dd64bc0ada74/include/axe_terminal.h#L526-L527) | convert the offset to `difference_type` once |

B1 (`A && B`) and N1 (one-argument extractor) could also be fixed in the code instead of the guide; B1 was (see *Resolution*).

## Recommendations

1. Apply the fixes above to `docs/AXE-Reference.md`; the conversion was kept faithful so that they show up as a reviewable diff.
2. Decide the version number and copyright line (A4) and regenerate the PDF with `make docs` (see `docs/README.md`).
3. Most of the broken examples would have been caught by compiling them. A test that extracts the ```` ```cpp ````
   blocks of the *Examples* chapter and compiles them, as was done for this review, would keep the guide and the code in step.
4. The test suite doesn't exercise `r_skip`, `r_buffered`, `r_convert`, `r_ucase`/`r_lcase`/`r_icase`, `r_bool` or
   `r_constrained`, all of which the guide documents.

## Notes on the Markdown conversion

`AXE-Reference.md` keeps the text, tables and examples of the PDF unchanged, including the issues above. Only artifacts of
the Word/PDF format were changed:

- typographic quotes and dashes in code replaced with ASCII (`“str”_axe` → `"str"_axe`, `A – B` → `A - B`);
- words hyphenated or wrapped by the PDF layout joined again (`".o."_re-gex`, the `r_bool` example, the wildcard text);
- code indentation normalized to four spaces;
- the page headers, footers and page numbers of the table of contents dropped (pandoc generates the table of contents);
- long code examples in table cells moved below the table ("Example 1: `r_fail(F)`" …), with a link from the cell, because
  Markdown table cells can't hold code blocks;
- footnotes kept as Markdown footnotes; the PDF prints them at the end of each chapter instead of the bottom of the page.
