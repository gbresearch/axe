#!/usr/bin/env python3
"""Compile the guide's actual C++ fences and verify its documented output.

Usage: python tools/check_docs.py --compiler g++-14 --std c++23
       python tools/check_docs.py --compiler cl --std latest

No third party Python packages or Yadro checkout are required. See
tools/doc_examples/README.md for the explicit fragment completion policy.
"""

import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


class CheckError(RuntimeError):
    """An actionable documentation verification failure."""


def digest(content):
    return hashlib.sha256(content.encode("utf-8")).hexdigest()


def extract_blocks(markdown):
    """Read fenced blocks, tracking headings outside fences and source lines."""
    blocks = []
    heading = ""
    fence = None
    for number, line in enumerate(markdown.splitlines(keepends=True), 1):
        if fence is not None:
            if re.fullmatch(r" {0,3}" + re.escape(fence[0]) + "{" + str(len(fence)) + r",}\s*", line):
                block["content"] = "".join(content)
                blocks.append(block)
                fence = None
            else:
                content.append(line)
            continue
        opening = re.match(r"^ {0,3}(`{3,}|~{3,})([^\r\n]*)", line)
        if opening:
            fence, info = opening.groups()
            language = info.strip().split()[0] if info.strip() else ""
            block = {"language": language, "heading": heading, "line": number + 1}
            content = []
        else:
            match = re.match(r"^ {0,3}#{1,6}\s+(.+?)\s*#*\s*$", line)
            if match:
                heading = match[1]
    if fence is not None:
        raise CheckError(f"unclosed fence at line {block['line'] - 1}")
    return blocks


def validate_manifest(blocks, manifest):
    """Require a one-to-one content-and-heading match for every fenced block."""
    entries = [(entry, "cpp") for entry in manifest["snippets"]]
    entries += [(entry, entry["language"]) for entry in manifest["other_blocks"]]
    matched = {}
    remaining = list(blocks)
    for entry, language in entries:
        name = entry["id"]
        if name in matched:
            raise CheckError(f"duplicate manifest id: {name}")
        candidates = [b for b in remaining if b["language"] == language
                      and b["heading"] == entry["heading"]
                      and digest(b["content"]) == entry["sha256"]]
        if not candidates:
            raise CheckError(f"changed or missing snippet/block {name!r} under heading {entry['heading']!r}; review the guide and update its explicit fixture manifest")
        found = candidates[0]
        matched[name] = found
        remaining.remove(found)
    if remaining:
        locations = ", ".join(f"{b['language'] or 'unlabelled'} at line {b['line']} ({b['heading']})" for b in remaining)
        raise CheckError(f"unaccounted fenced blocks: {locations}; add explicit manifest fixtures")
    return matched


def snippet_source(entry, matched):
    source = matched[entry["id"]]["content"]
    for replacement in entry.get("replacements", []):
        old, new = replacement["old"], replacement["new"]
        if not replacement.get("reason") or not old or source.count(old) != replacement["count"]:
            raise CheckError(f"{entry['id']}: substitution must match its recorded count and have a reason")
        if old == source or not source.replace(old, "").strip():
            raise CheckError(f"{entry['id']}: substitution cannot discard the entire snippet")
        source = source.replace(old, new)
    return source


def render_case(entry, matched, prelude, entries=None):
    dependencies = ""
    for name in entry.get("dependencies", []):
        if entries is None or name not in entries:
            raise CheckError(f"{entry['id']}: unknown dependency {name}")
        dependencies += snippet_source(entries[name], matched) + "\n"
    return (prelude + dependencies + entry.get("before", "")
            + snippet_source(entry, matched) + entry.get("after", ""))


def find_compiler(compiler):
    # The argument is one executable, never shell text or a whitespace-split path.
    invocation = shutil.which(compiler)
    if invocation is None:
        raise CheckError(f"compiler not found: {compiler}")
    # Preserve driver aliases: clang++ may be a symlink to clang, whose argv[0]
    # selects different default runtime linkage. Only make the path absolute.
    return str(Path(invocation).absolute())


def run_command(command, cwd, timeout, label):
    try:
        result = subprocess.run(command, cwd=cwd, timeout=timeout, capture_output=True,
                                text=True, encoding="utf-8", errors="replace")
    except subprocess.TimeoutExpired as exc:
        raise CheckError(f"{label} timed out after {timeout}s: {command!r}\n{exc.stdout or ''}\n{exc.stderr or ''}") from exc
    except OSError as exc:
        raise CheckError(f"{label} could not start: {command!r}: {exc}") from exc
    if result.returncode:
        diagnostics = (result.stdout + result.stderr).replace("\n", " | ")
        raise CheckError(f"{label} exited {result.returncode}: {command!r}\n{diagnostics}")
    return result


def compare_output(name, actual, expected):
    # Only platform line endings are normalized; spaces/tabs/blank lines matter.
    actual, expected = actual.replace("\r\n", "\n"), expected.replace("\r\n", "\n")
    if actual != expected:
        diff = "".join(difflib.unified_diff(expected.splitlines(keepends=True), actual.splitlines(keepends=True), fromfile="documented", tofile="actual"))
        raise CheckError(f"{name}: output mismatch\n{diff}")


def adjust_output(expected, adjustments):
    """Apply only explicit, counted presentation whitespace exceptions."""
    for adjustment in adjustments:
        old, new = adjustment["old"], adjustment["new"]
        if not old or not adjustment.get("reason") or expected.count(old) != adjustment["count"]:
            raise CheckError("output whitespace exception must match its recorded count and have a reason")
        if re.sub(r"[ \t]", "", old) != re.sub(r"[ \t]", "", new):
            raise CheckError("output exceptions may change only horizontal whitespace")
        expected = expected.replace(old, new)
    return expected


def compiler_standard(is_msvc, standard):
    return "c++latest" if is_msvc and standard in ("latest", "c++23") else standard


def warning_flags(is_msvc, compiler):
    if is_msvc:
        return ["/W3", "/WX"]
    flags = ["-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Wno-parentheses"]
    if "clang" in Path(compiler).name.lower():
        flags.append("-Wno-bitwise-op-parentheses")
    return flags


def verify(root, compiler, standard, timeout):
    fixtures = root / "tools" / "doc_examples"
    manifest = json.loads((fixtures / "manifest.json").read_text(encoding="utf-8"))
    document = root / "docs" / "AXE-Reference.md"
    matched = validate_manifest(extract_blocks(document.read_text(encoding="utf-8")), manifest)
    entries = {e["id"]: e for e in manifest["snippets"]}
    prelude = (fixtures / "prelude.hpp").read_text(encoding="utf-8")
    is_msvc = Path(compiler).name.lower() in ("cl", "cl.exe")
    standard = compiler_standard(is_msvc, standard)
    build = root / "build" / "doc_examples"
    build.mkdir(parents=True, exist_ok=True)
    # Fresh per-run directories prevent stale binaries and concurrent collisions.
    directory = Path(tempfile.mkdtemp(prefix=Path(compiler).stem + "-", dir=build))
    executed = compared = 0
    outputs_used = set()
    print(f"Documentation check: {len(entries)} C++ snippets; build files: {directory}", flush=True)
    for entry in manifest["snippets"]:
        name = entry["id"]
        source = directory / (name + ".cpp")
        binary = directory / (name + (".exe" if os.name == "nt" else ""))
        source.write_text(render_case(entry, matched, prelude, entries), encoding="utf-8")
        if is_msvc:
            command = [compiler, *warning_flags(True, compiler), "/nologo", "/EHsc", "/utf-8", "/permissive-", f"/std:{standard}",
                       "/I" + str(root / "include"), str(source), "/Fe:" + str(binary),
                       "/Fo:" + str(directory / (name + ".obj"))]
        else:
            command = [compiler, *warning_flags(False, compiler), "-std=" + standard, "-I", str(root / "include"), str(source), "-o", str(binary)]
        run_command(command, directory, timeout, f"{name} compiler")
        if entry.get("run", False):
            arguments = list(entry.get("args", []))
            if "input_file" in entry:
                arguments.append(str(fixtures / entry["input_file"]))
            result = run_command([str(binary), *arguments], directory, timeout, f"{name} example")
            executed += 1
            if "output" in entry:
                output_id = entry["output"]
                if output_id not in matched:
                    raise CheckError(f"{name}: unknown documented output {output_id}")
                expected = adjust_output(matched[output_id]["content"], entry.get("output_whitespace", []))
                outputs_used.add(output_id)
                compare_output(name, result.stdout, expected)
                compared += 1
            elif "expected_stdout" in entry:
                compare_output(name, result.stdout, entry["expected_stdout"])
            if result.stderr:
                raise CheckError(f"{name}: unexpected stderr: {result.stderr}")
        print(f"  PASS {name}" + (" (run)" if entry.get("run", False) else " (compile)"), flush=True)
    declared_outputs = {e["id"] for e in manifest["other_blocks"] if e["role"] == "output"}
    if outputs_used != declared_outputs:
        raise CheckError(f"unverified documented outputs: {sorted(declared_outputs - outputs_used)}")
    print(f"PASS: {len(entries)} snippets compiled; {executed} examples executed; {compared} documented outputs matched", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, help="compiler executable name or full path (one argument, including spaces)")
    parser.add_argument("--std", default="c++23", choices=["c++17", "c++20", "c++23", "latest"])
    parser.add_argument("--timeout", type=float, default=60, help="seconds per compiler or example process (default: 60)")
    options = parser.parse_args()
    try:
        if options.timeout <= 0:
            raise CheckError("timeout must be greater than zero")
        compiler = find_compiler(options.compiler)
        if options.std == "latest" and Path(compiler).name.lower() not in ("cl", "cl.exe"):
            raise CheckError("--std latest requires MSVC cl; use --std c++23 for GCC/Clang")
        verify(Path(__file__).resolve().parents[1], compiler, options.std, options.timeout)
    except (CheckError, KeyError, ValueError, OSError) as exc:
        print(f"Documentation check failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
