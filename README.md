# AXE

AXE is a C++ library that provides facilities to build recursive descent parsers. A recursive descent parser is a top-down parser built from a set of mutually-recursive procedures (or classes) where each such procedure (or class) implements one of the production rules of the grammar.
AXE library contains a set of classes, functions, and operators to define syntax rules and semantic actions. The library uses standard C++ and doesn’t require compiler/platform specific facilities.
AXE is a header only library, it doesn't require linking. You only need to add

```cpp
#include <axe.h>
```

in your source files and set the include directory in your compiler environment to point to `axe/include`.

## Requirements

- The minimum language level is C++17 (checked in `axe_macro.h`). C++20 is needed for `char8_t` input and for the compile-time string literals described below.
- The headers use only standard C++, without compiler extensions. They are tested with:
  - Visual Studio 2026 (MSVC toolset v145) with `/std:c++latest`, x64 Debug and Release;
  - GCC 14 and Clang 20 on Linux with `-std=c++23`, release and debug, at `-Wall -Wextra -Wpedantic -Werror`, and under AddressSanitizer, UndefinedBehaviorSanitizer and ThreadSanitizer.
- Each header also compiles on its own, warning-free at `-Wall -Wextra -Wpedantic`, with GCC and Clang at C++17, C++20 and C++23 (`make check-headers`). The library alone also works with older compilers, such as GCC 13 and Clang 18.
- The tests use the `gbtest` framework from [Yadro](https://github.com/gbresearch/yadro), which needs C++23: Visual Studio 2026, GCC 14 or later, or Clang 20 or later with libstdc++ 14 or later.

## Building and running the tests

The tests expect Yadro checked out next to this repository:

```sh
git clone https://github.com/gbresearch/axe
git clone https://github.com/gbresearch/yadro
cd axe
```

### Visual Studio

`vs/AXE.sln` builds and runs `vs/axe_test.vcxproj`, x64 Debug and Release.

### GCC or Clang on Linux

The `Makefile` builds the same test executable from the same sources. On Ubuntu 24.04 the compilers are `sudo apt install g++-14` or `sudo apt install clang-20`; `make` reports an error if the compiler is too old.

```sh
make -j8 test                       # build and run with g++, release (-O2)
make -j8 test TOOLCHAIN=clang       # with clang++
make -j8 test CXX=g++-14            # with a specific compiler
make -j8 test CONFIG=debug          # debug (-O0 -g)
make -j8 test WERROR=1              # warnings are errors
make test YADRO_DIR=path/to/yadro   # Yadro somewhere other than ../yadro
make check-headers CXXSTD=-std=c++17   # each header compiles on its own at a language level
make clean                          # remove this compiler's and configuration's outputs
make help                           # all options
```

- The executable is `build/<os>-<compiler>/<Config>/axe_test`, e.g. `build/linux-g++-14/Release/axe_test`. It runs in that directory and writes `axe-test.log` there.
- `TEST_ARGS` passes arguments to the test executable, which selects tests with `--suite <name>` and `--test <suite>.<name>` (the wildcards `*` and `?` are allowed), lists them with `--list`, and prints its options with `--help`:

  ```sh
  make test TEST_ARGS="--test axe.test_json*"
  make test TEST_ARGS=--list
  ```

- `CPPFLAGS`, `CXXFLAGS`, `LDFLAGS` and `LDLIBS` are added to the flags, e.g. for sanitizers:

  ```sh
  make -j8 test CONFIG=debug CXXFLAGS=-fsanitize=address,undefined LDFLAGS=-fsanitize=address,undefined
  ```

## Literal shortcuts: `_axe`, `_any`, `_regex`

`axe::shortcuts` (`axe_shortcut.h`) defines user-defined literals that create rules:

- `'c'_axe` (`char` or `wchar_t`) matches the character, as `r_char('c')`.
- `"abc"_axe` matches the string. With C++20 or later it is a standard string literal operator template: the characters become template arguments, `"abc"_axe` returns `r_strlit<'a', 'b', 'c'>`, and `""_axe` returns `r_empty`, which matches without consuming input. It works with `char`, `wchar_t`, `char8_t`, `char16_t` and `char32_t` literals, and characters after an embedded `\0` are matched too. With C++17 it returns `r_str` and works with `char` and `wchar_t` literals.
- `"abc"_any` matches any one of the characters, as `r_any("abc")`.
- `"[0-9]+"_regex` matches a regular expression, as `r_regex("[0-9]+")`.

```cpp
using namespace axe::shortcuts;
auto assignment = _ident & *_ws & "="_axe & *_ws & _int & ';'_axe;
auto escaped = '\\' & "nrt\\"_any;
```

## Limiting recursion depth: `r_depth_limit`

Recursive grammars (defined with `r_rule` and `std::ref`) recurse on the C++ stack, so deeply nested input, such as 100,000 `[` characters fed to a JSON grammar, can overflow the stack. `r_depth_limit` (`axe_depth.h`) wraps a rule and bounds its nesting depth:

```cpp
axe::r_rule<I> value;
auto array = '[' & *_ws & (']'_axe | (*_ws & std::ref(value) & *_ws) % ',' & ']');
value = axe::r_depth_limit(json_string | _double | array /* | ... */, 256);

try
{
    axe::parse(std::ref(value), begin, end);
}
catch (const axe::depth_limit_exceeded<I>& ex)
{
    // ex.position() - where the rejected invocation started
    // ex.max_depth() - the limit that was exceeded
}
```

- `r_depth_limit(rule, max_depth)` returns `r_depth_limit_t<Rule>`. Each invocation of the wrapper increments the depth for the duration of the match. When the depth exceeds `max_depth`, the wrapper throws `axe::depth_limit_exceeded<Iterator>` before invoking the wrapped rule. The outermost invocation has depth 1, and `max_depth()` returns the limit.
- `depth_limit_exceeded<Iterator>` (`axe_exception.h`) derives from `axe::failure<value_type>`, so existing handlers of `failure` also catch it. Catch it first to tell a depth violation apart from a syntax error.
- Every invocation counts, including one that fails to match. `'(' & ~r & ')'` invokes `r` one level deeper than it matches; `'(' & (r_char(')') | r & ')')` doesn't.
- The depth is kept per wrapper object and per thread. The same rule object can be used by several threads concurrently. Nested parses of the same rule on one thread (for example, started from a semantic action) are counted cumulatively. The count is restored on every exit path, including exceptions thrown by semantic actions.
- Put the wrapper at a single point that every recursive cycle passes through, typically the right-hand side of the `r_rule` assignment. Separate copies of a wrapper count separately.
- A parse must not be suspended (for example, by a fiber or coroutine switch) while inside the wrapper.
- The rules that don't use the wrapper are unchanged and have no overhead.
- Choose `max_depth` so that `max_depth` levels fit the stack of the parsing thread. Stack usage per level depends on the grammar and the build. For the JSON sample in `test/jason_test.cpp` it is about 40 KB per nesting level in an MSVC Debug build (`/ZI /JMC /RTC1`) and about 3 KB in Release, so a 1 MB thread stack holds roughly 20 levels in Debug and 300 in Release.

## `r_rule` inside rules that change the iterator type

`r_rule<I>` uses type erasure, so it can be called only with iterators convertible to `I`. `r_skip`, `r_convert` and `r_buffered` don't pass the input iterator to their sub-rule; they pass `skip_iterator<I, F>`, `convert_iterator<I, F>` and `input_buffer<I>::iterator` respectively. An `r_rule` used in the sub-rule, including through `std::ref`, must be declared with that iterator type, otherwise compilation fails at the `static_assert` in `r_rule::operator()`:

```cpp
std::string text = "abc [ abc [ 2 ] ]";
using I = std::string::iterator;
axe::r_rule<axe::skip_iterator<I, axe::r_pred<axe::is_wspace>>> r;  // the iterator created by r_skip(..., _ws)
auto a = axe::r_lit("abc") & std::ref(r);
r = '[' & (_int | a) & ']';
bool matched = axe::parse(axe::r_skip(a, _ws), text).matched;
```

- For `r_skip`, `F` must be exactly the skipper type it stores: the decayed type of a predicate or rule (`std::decay_t<decltype(_ws)>`, not `decltype(_ws)`), `is_char_t<C>` for a character, and `is_any_t<const C*>` for a string.
- Such a rule works only inside that transformation. A function object with a templated `operator()`, like `r_expression` in `axe_expression.h`, works with any iterator type. Alternatively, keep `r_rule<I>` and skip white space explicitly with `*_ws`.
- `r_skip` skips between every pair of characters, not only between tokens: `r_skip(r_decimal(n), _ws)` reads `1 2` as 12.

## Validating UTF-8: `r_utf8`, `r_utf8str`

`axe_utf8.h` provides terminal rules for well-formed UTF-8 (RFC 3629, Unicode Table 3-7):

- `r_utf8()` matches exactly one well-formed UTF-8 encoded code point. It rejects stray continuation bytes, overlong encodings, surrogate code points U+D800..U+DFFF encoded directly, code points above U+10FFFF, the bytes 0xC0, 0xC1 and 0xF5..0xFF, and truncated sequences. On failure the returned position is the start of the ill-formed sequence.
- `r_utf8str()` matches one or more well-formed code points and stops before the first ill-formed sequence or at the end of input.
- Both rules work with forward (multi-pass) iterators over `char`, `signed char`, `unsigned char` and `char8_t`. Single-pass input iterators such as `std::istreambuf_iterator` are rejected at compile time: advancing a copy of them moves the shared stream, so the start of a rejected sequence couldn't be restored. Read such input into a buffer first.

```cpp
bool valid = axe::parse(*axe::r_utf8() & axe::r_end(), text).matched;  // whole input is UTF-8
auto json_char = axe::r_utf8() - '"' - '\\' - axe::r_any('\x00', '\x1f')  // unescaped JSON character
    | '\\' & json_escaped;
```

## JSON sample

`test/jason_test.cpp` shows a JSON (RFC 8259) grammar built with these rules. Unescaped string characters must be well-formed UTF-8 and not control characters. UTF-16 escapes must pair a high surrogate with a low surrogate. Empty arrays and objects are accepted, any value is allowed at the top level, and nesting is bounded with `r_depth_limit`.
