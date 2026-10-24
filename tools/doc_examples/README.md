# Reference example checks

Run from the repository (Python 3, standard library only):

    python tools/check_docs.py --compiler g++-14 --std c++23
    python tools/check_docs.py --compiler clang++-20 --std c++23
    python tools/check_docs.py --compiler cl --std latest
    python -m unittest discover -s tools -p 'test_check_docs.py'

Initialize the Visual Studio developer environment before using `cl`. Compiler
paths containing spaces are a single quoted `--compiler` argument. A missing
compiler fails. `--timeout` bounds each compiler and example process (60 seconds
by default). GCC/Clang support `c++17`, `c++20`, and `c++23`; MSVC also accepts
`latest`, and the default `c++23` selects `/std:c++latest` on MSVC.

`manifest.json` accounts for every fenced block in `docs/AXE-Reference.md`:
44 C++ snippets, seven documented expected output blocks, one CSV input block,
and two grammar blocks. The manifest binds each block to its SHA-256 and heading,
so new, removed, moved, or changed snippets/outputs fail until their fixtures
are reviewed. These fingerprints describe LF-normalized UTF-8 Markdown as read
by Python; platform checkout line endings do not change them. There is no
regeneration or accept-current-output option in the checker.

Each C++ snippet produces a fresh translation unit from the **current Markdown**.
`prelude.hpp` supplies supporting includes, namespace imports, and a `require`
helper that aborts even if assertions are disabled. Manifest `before`/`after`
fields supply the surrounding scope, missing inputs, and a driver; dependencies
insert other current Markdown snippets. No example body is duplicated here.

All 44 translation units compile and execute their drivers. The complete word
counter runs its original `main` with `words.txt`. Some short inclusion or
signature snippets have an empty driver; declaration-only snippets check syntax,
while other fragments instantiate their shown rule with a small input. Seven
drivers compare stdout directly with the guide's expected fenced output. The
extractor-chain example additionally checks a fixture-provided expected string;
the guide does not give a separate expected-output block for that example.
Only CRLF/LF is normalized for output comparison; other whitespace remains
significant except for two explicit manifest exceptions: the JSON driver emits
`JSON object parsed: ` with one trailing space, and the Roman parser emits a
trailing space after its final integer; both are omitted in the documented
output. Each exception matches its exact line once, records the reason, and can
change only horizontal whitespace. Substantive output changes still fail.
Unexpected stderr, nonzero exit status, output differences, and timeouts fail.

Two snippets contain explicit pseudocode and need bounded substitutions:

* `recursive-pseudocode` supplies the four omitted stored references and
  constructors, then fills the two omitted expression continuations with
  `axe::r_empty()`. It instantiates the mutually recursive rules on `(123)`.
  This checks the shown recursive structure, not the omitted full arithmetic
  grammar.
* `write-xml-signature` replaces the prose token `parse-tree-data` with the
  concrete leaf type `axe::it_pair<const char*>`. This checks the declaration,
  while `write-xml` executes the real `axe::write_xml` API and verifies its output.

Two further declaration annotations address intentionally unused illustrative
bindings without disabling warnings: `depth-limit` marks the catch variable
`ex` as `[[maybe_unused]]` (its uses are explanatory comments), and `word-counter`
marks the stored parse result `p` as `[[maybe_unused]]`. These are distinct from
the pseudocode completions above and preserve the examples' behavior.

Each substitution records its exact source text, required occurrence count, and
reason. A mismatch fails; replacing/discarding an entire snippet is rejected.
The incomplete `parse-expression-signature` receives a body forwarding to the
actual AXE function, and the `get-as-signature` remains a declaration. These
completions are deliberately visible in the manifest; they do not alter the
guide or the library.

Compilation treats warnings as errors: GCC/Clang use `-Wall -Wextra -Wpedantic
-Werror`, with the intentional grammar-operator parentheses warnings suppressed
as in the test Makefile; Clang also suppresses `-Wbitwise-op-parentheses`. MSVC
uses `/W3 /WX`. Other compiler warnings fail verification.

Generated source, object files, and executables remain in fresh directories
under ignored `build/doc_examples/`. Failed runs keep their source for diagnosis;
concurrent invocations receive separate directories and cannot reuse stale
executables. Compiler diagnostics and child output are captured on failure.
