# AXE

AXE is a C++ library that provides facilities to build recursive descent parsers. A recursive descent parser is a top-down parser built from a set of mutually-recursive procedures (or classes) where each such procedure (or class) implements one of the production rules of the grammar.
AXE library contains a set of classes, functions, and operators to define syntax rules and semantic actions. The library uses standard C++ and doesn’t require compiler/platform specific facilities.
AXE is a header only library, it doesn't require linking. You only need to add

#include <axe.h>

in your source files and set the include directory in your compiler environment to point to axe/include.

## Requirements

- The minimum language level is C++17 (checked in `axe_macro.h`); C++20 is needed only for `char8_t` input.
- The library is developed and tested with Visual Studio 2026 (MSVC toolset v145) using `/std:c++latest`; the projects in `vs/AXE.sln` build x64 Debug and Release configurations. It is expected to work with any other reasonably compliant C++17 (or later) compiler.
- The test project `vs/axe_test.vcxproj` uses the `gbtest` framework from [Yadro](https://github.com/gbresearch/yadro), which must be checked out next to this repository (`../yadro`).

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

## Validating UTF-8: `r_utf8`, `r_utf8str`

`axe_utf8.h` provides terminal rules for well-formed UTF-8 (RFC 3629, Unicode Table 3-7):

- `r_utf8()` matches exactly one well-formed UTF-8 encoded code point. It rejects stray continuation bytes, overlong encodings, surrogate code points U+D800..U+DFFF encoded directly, code points above U+10FFFF, the bytes 0xC0, 0xC1 and 0xF5..0xFF, and truncated sequences. On failure the returned position is the start of the ill-formed sequence.
- `r_utf8str()` matches one or more well-formed code points and stops before the first ill-formed sequence or at the end of input.
- Both rules work with iterators over `char`, `signed char`, `unsigned char` and `char8_t`.

```cpp
bool valid = axe::parse(*axe::r_utf8() & axe::r_end(), text).matched;  // whole input is UTF-8
auto json_char = axe::r_utf8() - '"' - '\\' - axe::r_any('\x00', '\x1f')  // unescaped JSON character
    | '\\' & json_escaped;
```

## JSON sample

`test/jason_test.cpp` shows a JSON (RFC 8259) grammar built with these rules. Unescaped string characters must be well-formed UTF-8 and not control characters. UTF-16 escapes must pair a high surrogate with a low surrogate. Empty arrays and objects are accepted, any value is allowed at the top level, and nesting is bounded with `r_depth_limit`.
