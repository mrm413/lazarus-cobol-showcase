#!/usr/bin/env python3
"""
Lazarus C++17 — Oracle Parity Diagnostic Tool
Torsova LLC — lazarus-systems.com

Compiles and runs every non-stub transpiled C++ program that has expected
oracle output, then prints a clear diff for every mismatch.

Usage inside Docker:
  docker run --rm -v /path/to/federal-validator:/diag lazarus-cpp17-validator \
      python3 /diag/oracle_diagnose.py

Or copy into container:
  docker cp oracle_diagnose.py CONTAINER:/validator/oracle_diagnose.py
  docker exec CONTAINER python3 /validator/oracle_diagnose.py

Environment variables (all optional):
  CPP_DIR       — directory containing category/slug/slug_hardened.cpp
  ORACLE_JSON   — path to testsuite_oracle.json
  CXX           — compiler (default: g++)
  CXXFLAGS      — flags    (default: -std=c++17 -O2)
  TIMEOUT_SEC   — per-program timeout (default: 5)
"""

import json
import os
import re
import shlex
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

CPP_DIR      = os.environ.get("CPP_DIR",     "/validator/cpp_output")
ORACLE_JSON  = os.environ.get("ORACLE_JSON",  "/validator/testsuite_oracle.json")
CXX          = os.environ.get("CXX",          "g++")
CXXFLAGS     = os.environ.get("CXXFLAGS",     "-std=c++17 -O2")
TIMEOUT_SEC  = int(os.environ.get("TIMEOUT_SEC", "5"))

# Also try local paths when run outside Docker
if not os.path.isfile(ORACLE_JSON):
    local_json = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                              "testsuite_oracle.json")
    if os.path.isfile(local_json):
        ORACLE_JSON = local_json

if not os.path.isdir(CPP_DIR):
    # Try sibling output/ directory (repo layout)
    local_cpp = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             "..", "output")
    if os.path.isdir(local_cpp):
        CPP_DIR = local_cpp

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def normalize(text: str) -> str:
    """Normalize output for comparison: strip trailing/leading whitespace per
    line, collapse runs of spaces, remove blank lines."""
    lines = []
    for line in text.splitlines():
        line = line.strip()
        line = re.sub(r" +", " ", line)
        if line:
            lines.append(line)
    return "\n".join(lines)


def is_stub(cpp_path: str) -> bool:
    """Return True if the file is a stub (contains TODO marker with no
    std::cout)."""
    try:
        with open(cpp_path, "r", encoding="utf-8", errors="replace") as f:
            src = f.read()
        return "TODO: LAZARUS" in src and "std::cout" not in src
    except OSError:
        return True


def compile_cpp(cpp_path: str, bin_path: str) -> tuple[bool, str]:
    """Compile a C++ file. Returns (success, stderr)."""
    cmd = f"{CXX} {CXXFLAGS} -o {shlex.quote(bin_path)} {shlex.quote(cpp_path)}"
    try:
        result = subprocess.run(
            cmd, shell=True, capture_output=True, text=True, timeout=30
        )
        return result.returncode == 0, result.stderr
    except subprocess.TimeoutExpired:
        return False, "compilation timed out (30s)"


def run_binary(bin_path: str) -> tuple[str, str, int]:
    """Run a compiled binary. Returns (stdout, error_info, exit_code)."""
    try:
        result = subprocess.run(
            [bin_path],
            capture_output=True,
            text=True,
            timeout=TIMEOUT_SEC,
        )
        return result.stdout, "", result.returncode
    except subprocess.TimeoutExpired:
        return "", "TIMEOUT", -1
    except OSError as exc:
        return "", f"EXEC_ERROR: {exc}", -1


def classify_diff(expected_norm: str, actual_norm: str, actual_raw: str) -> str:
    """Classify the type of mismatch."""
    if not actual_raw.strip():
        return "EMPTY_OUTPUT"
    if expected_norm.lower() == actual_norm.lower():
        return "CASE_DIFF"
    # Check if one is a prefix of the other
    if actual_norm.startswith(expected_norm):
        return "EXTRA_OUTPUT"
    if expected_norm.startswith(actual_norm):
        return "TRUNCATED_OUTPUT"
    # Check if lines are reordered
    exp_lines = set(expected_norm.splitlines())
    act_lines = set(actual_norm.splitlines())
    if exp_lines == act_lines:
        return "LINE_ORDER_DIFF"
    # Check overlap ratio
    common = exp_lines & act_lines
    total = exp_lines | act_lines
    if total and len(common) / len(total) > 0.5:
        return "PARTIAL_MATCH"
    # Check if it looks like a formatting/spacing issue
    exp_nospace = re.sub(r"\s+", "", expected_norm)
    act_nospace = re.sub(r"\s+", "", actual_norm)
    if exp_nospace == act_nospace:
        return "WHITESPACE_ONLY"
    return "CONTENT_MISMATCH"


def first_n_lines(text: str, n: int = 10) -> str:
    """Return at most the first n lines, with a trailer if truncated."""
    lines = text.splitlines()
    if len(lines) <= n:
        return text
    return "\n".join(lines[:n]) + f"\n  ... ({len(lines) - n} more lines)"


# ---------------------------------------------------------------------------
# ANSI colors (disabled when not a tty)
# ---------------------------------------------------------------------------

if sys.stdout.isatty():
    C_RED    = "\033[0;31m"
    C_GREEN  = "\033[0;32m"
    C_YELLOW = "\033[1;33m"
    C_CYAN   = "\033[0;36m"
    C_BOLD   = "\033[1m"
    C_DIM    = "\033[2m"
    C_RESET  = "\033[0m"
else:
    C_RED = C_GREEN = C_YELLOW = C_CYAN = C_BOLD = C_DIM = C_RESET = ""


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    print(f"""
{C_BOLD}{'=' * 66}
  LAZARUS C++17 -- ORACLE PARITY DIAGNOSTIC
  Torsova LLC -- lazarus-systems.com
{'=' * 66}{C_RESET}
  Oracle JSON:  {ORACLE_JSON}
  C++ dir:      {CPP_DIR}
  Compiler:     {CXX} {CXXFLAGS}
  Timeout:      {TIMEOUT_SEC}s
""")

    # Load oracle
    if not os.path.isfile(ORACLE_JSON):
        print(f"{C_RED}ERROR: Oracle JSON not found: {ORACLE_JSON}{C_RESET}")
        return 1
    with open(ORACLE_JSON, "r", encoding="utf-8") as f:
        tests = json.load(f)

    # Filter to tests with expected output
    tests_with_output = [t for t in tests if t.get("expected_stdout")]
    print(f"  Tests with expected_stdout: {len(tests_with_output)}")

    # Counters
    n_total    = 0
    n_match    = 0
    n_mismatch = 0
    n_stub     = 0
    n_no_file  = 0
    n_compile_fail = 0
    n_timeout  = 0
    mismatches = []

    with tempfile.TemporaryDirectory(prefix="oracle_diag_") as tmpdir:
        for test in tests_with_output:
            category = test["category"]
            slug     = test["slug"]
            expected = test["expected_stdout"]
            key      = f"{category}/{slug}"

            cpp_file = os.path.join(CPP_DIR, category, slug,
                                    f"{slug}_hardened.cpp")
            n_total += 1

            # -- No file --
            if not os.path.isfile(cpp_file):
                n_no_file += 1
                continue

            # -- Stub --
            if is_stub(cpp_file):
                n_stub += 1
                continue

            # -- Compile --
            bin_dir = os.path.join(tmpdir, category)
            os.makedirs(bin_dir, exist_ok=True)
            bin_path = os.path.join(bin_dir, slug)

            ok, compile_err = compile_cpp(cpp_file, bin_path)
            if not ok:
                n_compile_fail += 1
                mismatches.append({
                    "key": key,
                    "type": "COMPILE_FAIL",
                    "expected": expected,
                    "actual": "",
                    "detail": compile_err.strip()[:500],
                })
                continue

            # -- Run --
            actual, run_err, exit_code = run_binary(bin_path)
            if run_err == "TIMEOUT":
                n_timeout += 1
                mismatches.append({
                    "key": key,
                    "type": "TIMEOUT",
                    "expected": expected,
                    "actual": "",
                    "detail": f"Program did not finish within {TIMEOUT_SEC}s",
                })
                continue
            if run_err:
                mismatches.append({
                    "key": key,
                    "type": "RUN_ERROR",
                    "expected": expected,
                    "actual": "",
                    "detail": run_err[:500],
                })
                n_mismatch += 1
                continue

            # -- Compare --
            norm_exp = normalize(expected)
            norm_act = normalize(actual)

            if norm_exp == norm_act:
                n_match += 1
            else:
                diff_type = classify_diff(norm_exp, norm_act, actual)
                n_mismatch += 1
                mismatches.append({
                    "key": key,
                    "type": diff_type,
                    "expected": expected,
                    "actual": actual,
                    "detail": "",
                })

    # ---------------------------------------------------------------------------
    # Print diagnostics for every mismatch
    # ---------------------------------------------------------------------------

    if mismatches:
        print(f"\n{C_BOLD}{'=' * 66}")
        print(f"  MISMATCHES: {len(mismatches)} programs")
        print(f"{'=' * 66}{C_RESET}\n")

        # Group by type for summary
        by_type: dict[str, list] = {}
        for m in mismatches:
            by_type.setdefault(m["type"], []).append(m)

        print(f"  {C_BOLD}Breakdown by failure type:{C_RESET}")
        for t, items in sorted(by_type.items(), key=lambda x: -len(x[1])):
            print(f"    {t:25s}  {len(items):3d}")
        print()

        # Detailed output per mismatch
        for i, m in enumerate(mismatches, 1):
            key       = m["key"]
            diff_type = m["type"]
            expected  = m["expected"]
            actual    = m["actual"]
            detail    = m["detail"]

            sep = "-" * 66
            print(f"{C_BOLD}{sep}{C_RESET}")
            print(f"  {C_YELLOW}[{i}/{len(mismatches)}]{C_RESET}  "
                  f"{C_BOLD}{key}{C_RESET}")
            print(f"  Type: {C_RED}{diff_type}{C_RESET}")

            if detail:
                print(f"  Detail: {C_DIM}{detail}{C_RESET}")

            print()
            print(f"  {C_GREEN}EXPECTED (oracle):{C_RESET}")
            exp_preview = first_n_lines(expected, 10)
            for line in exp_preview.splitlines():
                print(f"    {C_GREEN}| {line}{C_RESET}")

            if actual or diff_type not in ("COMPILE_FAIL", "TIMEOUT",
                                           "RUN_ERROR"):
                print()
                print(f"  {C_RED}ACTUAL (C++17):{C_RESET}")
                if actual.strip():
                    act_preview = first_n_lines(actual, 10)
                    for line in act_preview.splitlines():
                        print(f"    {C_RED}| {line}{C_RESET}")
                else:
                    print(f"    {C_DIM}(empty){C_RESET}")

            # Show first divergent line for content mismatches
            if diff_type in ("CONTENT_MISMATCH", "PARTIAL_MATCH"):
                exp_lines = normalize(expected).splitlines()
                act_lines = normalize(actual).splitlines()
                for j, (el, al) in enumerate(
                        zip(exp_lines, act_lines)):
                    if el != al:
                        print()
                        print(f"  {C_CYAN}First divergence at "
                              f"line {j + 1}:{C_RESET}")
                        print(f"    exp: {el!r}")
                        print(f"    got: {al!r}")
                        break

            print()

    # ---------------------------------------------------------------------------
    # Summary
    # ---------------------------------------------------------------------------

    tested = n_match + n_mismatch + n_compile_fail + n_timeout
    print(f"{C_BOLD}{'=' * 66}")
    print(f"  DIAGNOSTIC SUMMARY")
    print(f"{'=' * 66}{C_RESET}")
    print(f"  Tests with expected output:  {n_total}")
    print(f"  No C++ file found:           {n_no_file}")
    print(f"  Stubs (not transpiled):       {n_stub}")
    print(f"  {C_GREEN}Exact match:{C_RESET}                {n_match}")
    print(f"  {C_RED}Mismatch:{C_RESET}                   {n_mismatch}")
    print(f"  {C_RED}Compile failures:{C_RESET}           {n_compile_fail}")
    print(f"  {C_YELLOW}Timeouts:{C_RESET}                  {n_timeout}")
    if tested > 0:
        rate = n_match * 100.0 / tested
        print(f"\n  {C_BOLD}Parity rate (match/tested):  "
              f"{rate:.1f}%  ({n_match}/{tested}){C_RESET}")
    print()

    return 0 if n_mismatch == 0 and n_compile_fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
