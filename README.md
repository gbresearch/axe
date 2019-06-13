# AXE

AXE is a C++ library that provides facilities to build recursive descent parsers. A recursive descent parser is a top-down parser built from a set of mutually-recursive procedures (or classes) where each such procedure (or class) implements one of the production rules of the grammar.
AXE library contains a set of classes, functions, and operators to define syntax rules and semantic actions. The library uses standard C++17 and doesn’t require compiler/platform specific facilities. It's been tested with Visual C++ 2017 and expected to work with any other reasonably compliant C++17 compiler.
AXE is a header only library, it doesn't require linking. You only need to add

#include <axe.h>

in your source files and set the include directory in your compiler environment to point to axe/include.
