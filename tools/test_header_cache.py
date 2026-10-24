#!/usr/bin/env python3
"""Exercise Make's header cache with a real compiler in disposable checkouts."""

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


class HeaderCacheTests(unittest.TestCase):
    compiler = "g++"
    make = "make"

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="axe-header-cache-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        shutil.copyfile(Path(__file__).resolve().parents[1] / "Makefile", self.root / "Makefile")
        (self.root / "include").mkdir()

    def check_headers(self, standard, **flags):
        return subprocess.run(
            [self.make, "--no-print-directory", "check-headers", f"CXX={self.compiler}",
             f"CXXSTD=-std={standard}", "PLATFORM=cache-regression",
             *(f"{key}={value}" for key, value in flags.items())],
            cwd=self.root, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=60,
        )

    def assert_recompiled(self, source, first, second, **flags):
        (self.root / "include" / "cache_probe.h").write_text(source, encoding="utf-8")
        initial = self.check_headers(first)
        self.assertEqual(initial.returncode, 0, initial.stdout)
        changed = self.check_headers(second, **flags)
        self.assertNotEqual(changed.returncode, 0,
                            "Changed configuration silently reused a passing header check:\n" + changed.stdout)
        self.assertIn("AXE_CACHE_RECOMPILED", changed.stdout)

    def test_cxx17_to_cxx20_recompiles(self):
        self.assert_recompiled(
            '#if __cplusplus >= 202002L\n#error AXE_CACHE_RECOMPILED\n#endif\n',
            "c++17", "c++20",
        )

    def test_cxx20_to_cxx23_recompiles(self):
        self.assert_recompiled(
            '#if __cplusplus > 202002L\n#error AXE_CACHE_RECOMPILED\n#endif\n',
            "c++20", "c++23",
        )

    def test_cppflags_change_recompiles(self):
        self.assert_recompiled(
            '#ifdef AXE_CACHE_REJECT\n#error AXE_CACHE_RECOMPILED\n#endif\n',
            "c++23", "c++23", CPPFLAGS="-DAXE_CACHE_REJECT=1",
        )

    def test_cxxflags_change_recompiles(self):
        self.assert_recompiled(
            '#ifdef AXE_CACHE_REJECT\n#error AXE_CACHE_RECOMPILED\n#endif\n',
            "c++23", "c++23", CXXFLAGS="-DAXE_CACHE_REJECT=1",
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--make", default="make")
    options, remaining = parser.parse_known_args()
    if not shutil.which(options.make):
        parser.error(f"GNU make not found: {options.make}")
    HeaderCacheTests.compiler = options.compiler
    HeaderCacheTests.make = options.make
    unittest.main(argv=[sys.argv[0], *remaining])
