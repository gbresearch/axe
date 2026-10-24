"""Regression tests for documentation extraction and process/output failures."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).with_name("check_docs.py")


class MissingCompilerCLI(unittest.TestCase):
    def test_required_compiler_fails_loudly(self):
        result = subprocess.run([sys.executable, str(SCRIPT), "--compiler", "nonexistent-doc-compiler-25481"], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("compiler not found", result.stderr)


class HarnessRegressions(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location("check_docs", SCRIPT)
        cls.docs = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = cls.docs
        spec.loader.exec_module(cls.docs)

    def fixture(self, code="int main() {}\n"):
        markdown = "# Demo\n\n```cpp\n" + code + "```\n"
        blocks = self.docs.extract_blocks(markdown)
        manifest = {"snippets": [{"id": "demo", "heading": "Demo", "sha256": self.docs.digest(code), "before": "", "after": ""}], "other_blocks": []}
        return markdown, blocks, manifest

    def test_extract_fences_and_ignore_fake_heading_in_cpp(self):
        blocks = self.docs.extract_blocks("# Real\n```cpp\n#include <axe.h>\n```\n~~~text\nexpected\n~~~\n")
        self.assertEqual([(x["language"], x["heading"]) for x in blocks], [("cpp", "Real"), ("text", "Real")])
        self.assertEqual(blocks[0]["line"], 3)

    def test_unclosed_fence_fails(self):
        with self.assertRaisesRegex(self.docs.CheckError, "unclosed"):
            self.docs.extract_blocks("# Heading\n```cpp\nint x;")

    def test_new_snippet_fails(self):
        markdown, _, manifest = self.fixture()
        blocks = self.docs.extract_blocks(markdown + "\n```cpp\nint extra;\n```\n")
        with self.assertRaisesRegex(self.docs.CheckError, "unaccounted"):
            self.docs.validate_manifest(blocks, manifest)

    def test_changed_snippet_fails(self):
        _, _, manifest = self.fixture()
        _, blocks, _ = self.fixture("int main() { return 1; }\n")
        with self.assertRaisesRegex(self.docs.CheckError, "changed|unaccounted"):
            self.docs.validate_manifest(blocks, manifest)

    def test_relocated_heading_fails(self):
        _, blocks, manifest = self.fixture()
        blocks[0]["heading"] = "Other"
        with self.assertRaisesRegex(self.docs.CheckError, "heading|unaccounted"):
            self.docs.validate_manifest(blocks, manifest)

    def test_output_change_requires_manifest_review(self):
        _, blocks, manifest = self.fixture()
        manifest["other_blocks"] = [{"id": "output", "language": "text", "heading": "Demo", "sha256": self.docs.digest("old\n"), "role": "output"}]
        blocks += self.docs.extract_blocks("# Demo\n```text\nnew\n```\n")
        with self.assertRaisesRegex(self.docs.CheckError, "changed|unaccounted"):
            self.docs.validate_manifest(blocks, manifest)

    def test_extract_actual_current_source_into_wrapper(self):
        _, blocks, manifest = self.fixture("int fresh = 73;\n")
        matched = self.docs.validate_manifest(blocks, manifest)
        manifest["snippets"][0].update(before="// prefix\n", after="// suffix\n")
        rendered = self.docs.render_case(manifest["snippets"][0], matched, "// prelude\n")
        self.assertIn("int fresh = 73;", rendered)
        self.assertTrue(rendered.startswith("// prelude\n// prefix\n"))

    def test_substitution_must_match_exact_count(self):
        _, blocks, manifest = self.fixture()
        matched = self.docs.validate_manifest(blocks, manifest)
        manifest["snippets"][0]["replacements"] = [{"old": "not present", "new": "replacement", "count": 1, "reason": "test"}]
        with self.assertRaisesRegex(self.docs.CheckError, "substitution"):
            self.docs.render_case(manifest["snippets"][0], matched, "")

    def test_compiler_failure_captures_status_and_diagnostic(self):
        with self.assertRaisesRegex(self.docs.CheckError, "compiler.*17.*syntax failure"):
            self.docs.run_command([sys.executable, "-c", "import sys; print('syntax failure', file=sys.stderr); sys.exit(17)"], Path.cwd(), 5, "compiler")

    def test_child_timeout_fails(self):
        with self.assertRaisesRegex(self.docs.CheckError, "timed out"):
            self.docs.run_command([sys.executable, "-c", "import time; time.sleep(10)"], Path.cwd(), 0.05, "example")

    def test_output_mismatch_fails(self):
        with self.assertRaisesRegex(self.docs.CheckError, "output mismatch"):
            self.docs.compare_output("example", "wrong\n", "documented\n")

    def test_bounded_output_whitespace_exception_preserves_content_checks(self):
        adjustment = {"old": "JSON object parsed:\n", "new": "JSON object parsed: \n", "count": 1, "reason": "Guide omits the emitted trailing space."}
        expected = self.docs.adjust_output("JSON object parsed:\n", [adjustment])
        self.assertEqual(expected, "JSON object parsed: \n")
        with self.assertRaisesRegex(self.docs.CheckError, "output mismatch"):
            self.docs.compare_output("json", "JSON object failed: \n", expected)
        with self.assertRaisesRegex(self.docs.CheckError, "count"):
            self.docs.adjust_output("new documented line\n", [adjustment])
        adjustment["new"] = "JSON object failed: \n"
        with self.assertRaisesRegex(self.docs.CheckError, "whitespace"):
            self.docs.adjust_output("JSON object parsed:\n", [adjustment])

    def test_output_comparison_preserves_spaces(self):
        with self.assertRaises(self.docs.CheckError):
            self.docs.compare_output("example", "value \n", "value\n")
        self.docs.compare_output("example", "value\r\n", "value\n")

    def test_compiler_warnings_are_errors_with_only_intentional_suppressions(self):
        self.assertEqual(self.docs.warning_flags(True, "cl.exe"), ["/W3", "/WX"])
        gcc = self.docs.warning_flags(False, "g++-14")
        clang = self.docs.warning_flags(False, "clang++-20")
        for options in (gcc, clang):
            self.assertTrue({"-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Wno-parentheses"}.issubset(options))
        self.assertNotIn("-Wno-bitwise-op-parentheses", gcc)
        self.assertIn("-Wno-bitwise-op-parentheses", clang)

    def test_msvc_latest_uses_actual_compiler_switch(self):
        self.assertEqual(self.docs.compiler_standard(True, "latest"), "c++latest")
        self.assertEqual(self.docs.compiler_standard(True, "c++23"), "c++latest")
        self.assertEqual(self.docs.compiler_standard(True, "c++17"), "c++17")
        self.assertEqual(self.docs.compiler_standard(False, "c++23"), "c++23")

    def test_compiler_path_with_spaces_is_single_argument(self):
        with tempfile.TemporaryDirectory(prefix="docs compiler space ") as directory:
            target = Path(directory) / "compiler with spaces.exe"
            target.write_bytes(b"placeholder")
            target.chmod(0o755)
            # Keep invocation spelling, including Windows temporary 8.3 paths.
            self.assertEqual(self.docs.find_compiler(str(target)), str(target.absolute()))

    def test_compiler_symlink_invocation_alias_is_preserved(self):
        with tempfile.TemporaryDirectory(prefix="docs compiler alias ") as directory:
            backend = Path(directory) / "compiler-backend.exe"
            backend.write_bytes(b"placeholder")
            backend.chmod(0o755)
            alias = Path(directory) / "compiler-cxx.exe"
            try:
                alias.symlink_to(backend)
            except OSError as exc:
                if sys.platform == "win32" and getattr(exc, "winerror", None) == 1314:
                    self.skipTest("Windows denies symlink creation; this regression must also run on WSL/Linux")
                raise
            self.assertTrue(alias.is_symlink())
            self.assertNotEqual(alias.name, alias.resolve().name)
            self.assertEqual(self.docs.find_compiler(str(alias)), str(alias.absolute()))

    def test_fixture_cannot_silently_replace_entire_snippet(self):
        _, blocks, manifest = self.fixture()
        matched = self.docs.validate_manifest(blocks, manifest)
        manifest["snippets"][0]["replacements"] = [{"old": blocks[0]["content"], "new": "", "count": 1, "reason": "discard"}]
        with self.assertRaisesRegex(self.docs.CheckError, "entire snippet"):
            self.docs.render_case(manifest["snippets"][0], matched, "")


if __name__ == "__main__":
    unittest.main()
