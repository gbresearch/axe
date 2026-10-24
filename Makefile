#-----------------------------------------------------------------------------
#  Copyright (C) 2011-2026, Gene Bushuyev
#
#  Boost Software License - Version 1.0 - August 17th, 2003
#  (see the LICENSE file)
#-----------------------------------------------------------------------------
#
# GNU Makefile that builds and runs the AXE tests with GCC or Clang on Linux, the counterpart of
# vs/axe_test.vcxproj. AXE itself is header only; this builds the test executable (axe_test) from the
# same sources as the Visual Studio project.
#
#   make -j8                      test executable, GCC, release
#   make -j8 TOOLCHAIN=clang      the same with clang++
#   make -j8 CXX=g++-15           a specific compiler (GCC or Clang is detected from its --version)
#   make -j8 CONFIG=debug         debug build (-O0 -g -D_DEBUG) instead of release (-O2 -DNDEBUG)
#   make -j8 WERROR=1             warnings are errors
#   make test                     build and run the tests; TEST_ARGS="--suite axe" passes arguments
#   make check-headers            each header under include/ compiles on its own
#   make docs                     the Library Reference PDF from docs/AXE-Reference.md (pandoc, Chrome)
#   make clean                    remove the outputs of this compiler and configuration
#   make clean-all                remove the outputs of every compiler and configuration on this OS
#   make help                     this text
#
# Requirements: -std=c++23. The tests use the gbtest framework from Yadro
# (https://github.com/gbresearch/yadro), which is header only for this purpose but needs GCC 14 or
# later, or Clang 20 or later with libstdc++ 14 or later (the standard library Clang uses on Linux).
# Yadro is expected in a sibling checkout, ../yadro, as for the Visual Studio projects; YADRO_DIR=path
# points elsewhere (the directory itself must be named yadro, the tests include <yadro/util/gbtest.h>).
# gbtest uses std::stacktrace, which needs libstdc++exp (STACKTRACE_LIBS).
#
# The AXE headers alone need only C++17 (C++20 for char8_t input); check-headers compiles them with
# CXXSTD, so e.g. make check-headers CXXSTD=-std=c++17 checks the older language levels.
#
# CPPFLAGS, CXXFLAGS, LDFLAGS and LDLIBS given on the command line are added to the flags below, e.g.
# make CXXFLAGS=-fsanitize=address,undefined LDFLAGS=-fsanitize=address,undefined.
#
# Outputs, where PLATFORM is <os>-<compiler>, e.g. linux-g++-14, and Config is Release or Debug:
#   build/<PLATFORM>/<Config>/axe_test        the tests run here and write axe-test.log
#   build/<PLATFORM>/<Config>/obj/...
#   build/<PLATFORM>/header_check/...
#   build/docs/AXE-Reference.pdf              make docs, see docs/README.md
#-----------------------------------------------------------------------------

.DEFAULT_GOAL := all

TOOLCHAIN ?= gcc
CONFIG    ?= release

ifeq ($(origin CXX),default)
  ifeq ($(TOOLCHAIN),clang)
    CXX := clang++
  else ifeq ($(TOOLCHAIN),gcc)
    CXX := g++
  else
    $(error TOOLCHAIN must be gcc or clang, not '$(TOOLCHAIN)')
  endif
endif

COMPILER_ID    := $(if $(findstring clang,$(shell $(CXX) --version 2>/dev/null)),clang,gcc)
COMPILER_MAJOR := $(firstword $(subst ., ,$(shell $(CXX) -dumpversion 2>/dev/null)))
MIN_MAJOR      := $(if $(filter clang,$(COMPILER_ID)),20,14)

# an older compiler fails with errors deep in the Yadro headers, so say what is needed instead
ifneq ($(filter-out help clean clean-all check-headers docs,$(or $(MAKECMDGOALS),all)),)
  ifeq ($(shell [ "$(COMPILER_MAJOR)" -ge $(MIN_MAJOR) ] 2>/dev/null && echo ok),)
    $(error $(CXX) is $(COMPILER_ID) $(or $(COMPILER_MAJOR),of unknown version); the tests (Yadro gbtest) need GCC 14 or Clang 20 or later, e.g. make CXX=g++-14)
  endif
endif

ifeq ($(CONFIG),release)
  CONFIG_DIR   := Release
  CONFIG_FLAGS := -O2 -DNDEBUG
else ifeq ($(CONFIG),debug)
  CONFIG_DIR   := Debug
  CONFIG_FLAGS := -O0 -g -D_DEBUG
else
  $(error CONFIG must be release or debug, not '$(CONFIG)')
endif

OS_NAME  := $(shell uname -s | tr '[:upper:]' '[:lower:]')
PLATFORM ?= $(OS_NAME)-$(notdir $(firstword $(CXX)))

#-----------------------------------------------------------------------------
# Yadro

YADRO_DIR ?= ../yadro
YADRO_HEADER := $(YADRO_DIR)/util/gbtest.h

ifneq ($(filter-out help clean clean-all check-headers docs,$(or $(MAKECMDGOALS),all)),)
  ifeq ($(wildcard $(YADRO_HEADER)),)
    $(error $(YADRO_HEADER) not found: clone https://github.com/gbresearch/yadro next to this repository, or pass YADRO_DIR=<path to yadro>)
  endif
  ifneq ($(notdir $(abspath $(YADRO_DIR))),yadro)
    $(error YADRO_DIR ($(YADRO_DIR)) must be a directory named yadro: the tests include <yadro/util/gbtest.h>)
  endif
endif

# Yadro's headers are included as system headers, so their warnings don't hide the ones in AXE
YADRO_FLAGS := -isystem $(patsubst %/,%,$(dir $(abspath $(YADRO_DIR))))

#-----------------------------------------------------------------------------
# flags

WARN_FLAGS ?= -Wall -Wextra -Wpedantic
ifeq ($(WERROR),1)
  WARN_FLAGS += -Werror
endif

# The grammars in the tests mix the AXE operators & (sequence), | (alternative) and - (difference)
# without parentheses, relying on C++ operator precedence, which is how AXE grammars are written. The
# headers turn the corresponding warnings off for themselves (axe.h); the test sources need the same.
ifeq ($(COMPILER_ID),clang)
  TEST_WARN_FLAGS ?= -Wno-bitwise-op-parentheses -Wno-parentheses
else
  TEST_WARN_FLAGS ?= -Wno-parentheses
endif

STACKTRACE_LIBS ?= -lstdc++exp

CXXSTD     ?= -std=c++23
AXE_FLAGS   = -Iinclude $(CPPFLAGS) $(CXXSTD) $(CONFIG_FLAGS) -pthread
DEPFLAGS    = -MMD -MP

#-----------------------------------------------------------------------------
# sources: the same as in vs/axe_test.vcxproj

TEST_SOURCES := \
    test/cmd_test.cpp \
    test/cvs_test.cpp \
    test/depth_test.cpp \
    test/expression_test.cpp \
    test/format_test.cpp \
    test/ini_test.cpp \
    test/jason_test.cpp \
    test/literal_test.cpp \
    test/reference.cpp \
    test/replacement_test.cpp \
    test/roman_numerals.cpp \
    test/rules_test.cpp \
    test/utf8_test.cpp \
    test/wildcard_test.cpp \
    test/winpath_test.cpp \
    test/zip_test.cpp

BUILD_DIR    := build/$(PLATFORM)
EXE_DIR      := $(BUILD_DIR)/$(CONFIG_DIR)
TEST_OBJ_DIR := $(EXE_DIR)/obj
CHECK_DIR    := $(BUILD_DIR)/header_check

TEST_EXE     := $(EXE_DIR)/axe_test
TEST_OBJECTS := $(TEST_SOURCES:%.cpp=$(TEST_OBJ_DIR)/%.o)

#-----------------------------------------------------------------------------
# build

.PHONY: all test check-headers docs clean clean-all help

all: $(TEST_EXE)

$(TEST_EXE): $(TEST_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) -pthread $(LDFLAGS) -o $@ $(TEST_OBJECTS) $(STACKTRACE_LIBS) $(LDLIBS)

$(TEST_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(AXE_FLAGS) $(YADRO_FLAGS) $(WARN_FLAGS) $(TEST_WARN_FLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

test: $(TEST_EXE)
	cd $(EXE_DIR) && ./axe_test $(TEST_ARGS)

#-----------------------------------------------------------------------------
# check-headers: compiles, as a syntax and semantic check at -Wall -Wextra -Wpedantic -Werror, one
# translation unit per header under include/ that holds only #include <header.h>. It doesn't need Yadro.

CHECK_HEADERS := $(sort $(wildcard include/*.h))
CHECK_UNITS   := $(CHECK_HEADERS:include/%.h=$(CHECK_DIR)/%.h.ok)
CHECK_FLAGS    = -Iinclude $(CPPFLAGS) $(CXXSTD) -Wall -Wextra -Wpedantic -Werror $(CXXFLAGS) \
                 -fsyntax-only -MMD -MP -MF $@.d -MT $@

check-headers: $(CHECK_UNITS)
	@echo "check-headers: all $(words $(CHECK_UNITS)) headers compiled ($(CXX) $(CXXSTD))"

$(CHECK_DIR)/%.h.ok: include/%.h
	@mkdir -p $(@D)
	@printf '#include <%s>\n' $*.h > $(CHECK_DIR)/$*.h.cpp
	$(CXX) $(CHECK_FLAGS) $(CHECK_DIR)/$*.h.cpp
	@touch $@

#-----------------------------------------------------------------------------
# docs: the Library Reference PDF. pandoc converts the GitHub-flavored Markdown source to HTML and a
# Chromium-based browser (Chrome, Chromium or Edge 131 or later) prints the HTML to PDF. CHROME names the
# browser; CHROME_FLAGS adds options, e.g. CHROME_FLAGS=--no-sandbox when running as root in a container.

PANDOC       ?= pandoc
CHROME       ?= $(firstword $(foreach b,google-chrome google-chrome-stable chromium chromium-browser microsoft-edge,$(shell command -v $(b) 2>/dev/null)))
CHROME_FLAGS ?=

DOCS_DIR     := build/docs
DOCS_SOURCE  := docs/AXE-Reference.md
DOCS_HTML    := $(DOCS_DIR)/AXE-Reference.html
DOCS_PDF     := $(DOCS_DIR)/AXE-Reference.pdf

docs: $(DOCS_PDF)

$(DOCS_HTML): $(DOCS_SOURCE) docs/reference-template.html docs/reference.css
	@mkdir -p $(@D)
	$(PANDOC) $(DOCS_SOURCE) -f gfm -t html5 --standalone --embed-resources \
	    --template=docs/reference-template.html --css=docs/reference.css \
	    --toc --toc-depth=2 --reference-location=section --highlight-style=tango -o $@

$(DOCS_PDF): $(DOCS_HTML)
	$(if $(CHROME),,$(error no Chrome, Chromium or Edge found: pass CHROME=<path to the browser>))
	"$(CHROME)" --headless --disable-gpu --no-pdf-header-footer --generate-pdf-document-outline \
	    $(CHROME_FLAGS) --print-to-pdf="$(abspath $@)" "file://$(abspath $<)"

#-----------------------------------------------------------------------------

clean:
	rm -rf $(EXE_DIR) $(CHECK_DIR)

clean-all:
	rm -rf build/$(OS_NAME)-* $(DOCS_DIR)

help:
	@sed -n '/^# GNU Makefile/,/^#----/p' $(firstword $(MAKEFILE_LIST)) | sed -e '$$d' -e 's/^# \{0,1\}//'

-include $(TEST_OBJECTS:.o=.d) $(CHECK_UNITS:=.d)
