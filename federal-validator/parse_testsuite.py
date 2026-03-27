#!/usr/bin/env python3
"""
Parse GnuCOBOL .at test suite files and extract test cases with expected output.
Outputs a JSON manifest mapping test names to expected stdout.

Torsova LLC — lazarus-systems.com
"""

import json
import os
import re
import sys
import unicodedata


def slugify(name):
    """Convert AT_SETUP test name to our file naming convention."""
    s = name.lower()
    s = s.replace("'", "")
    s = s.replace('"', "")
    s = re.sub(r"[^a-z0-9]+", "_", s)
    s = s.strip("_")
    return s


def parse_at_file(filepath, category):
    """Parse a single .at file and extract test cases."""
    with open(filepath, "r", encoding="utf-8", errors="replace") as f:
        content = f.read()

    tests = []
    # Split on AT_SETUP
    blocks = re.split(r"AT_SETUP\(\[", content)

    for block in blocks[1:]:  # skip preamble
        # Extract test name
        name_match = re.match(r"([^\]]+)\]", block)
        if not name_match:
            continue
        test_name = name_match.group(1).strip()
        slug = slugify(test_name)

        # Find AT_CHECK with COBCRUN_DIRECT (runtime check)
        # Pattern: AT_CHECK([$COBCRUN_DIRECT ./prog], [exit_code],
        #          [expected_stdout], [expected_stderr])
        runtime_checks = re.findall(
            r"AT_CHECK\(\[\$COBCRUN_DIRECT\s+\./prog\w*\]\s*,\s*\[(\d+)\]\s*,\s*\[(.*?)\]\s*(?:,\s*\[(.*?)\])?\s*\)",
            block,
            re.DOTALL,
        )

        if not runtime_checks:
            # Also try plain COBCRUN
            runtime_checks = re.findall(
                r"AT_CHECK\(\[\$COBCRUN\s+prog\w*\]\s*,\s*\[(\d+)\]\s*,\s*\[(.*?)\]\s*(?:,\s*\[(.*?)\])?\s*\)",
                block,
                re.DOTALL,
            )

        expected_stdout = ""
        expected_exit = 0

        if runtime_checks:
            # Take the last runtime check (usually the main one)
            expected_exit = int(runtime_checks[-1][0])
            expected_stdout = runtime_checks[-1][1]
            # Clean up: remove leading/trailing newlines
            expected_stdout = expected_stdout.strip("\n")

        tests.append(
            {
                "name": test_name,
                "slug": slug,
                "category": category,
                "expected_stdout": expected_stdout,
                "expected_exit": expected_exit,
                "has_runtime_check": len(runtime_checks) > 0,
            }
        )

    return tests


def main():
    if len(sys.argv) < 2:
        print("Usage: parse_testsuite.py <testsuite.src_dir> [output.json]")
        sys.exit(1)

    src_dir = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else "testsuite_oracle.json"

    all_tests = []
    for filename in sorted(os.listdir(src_dir)):
        if not filename.endswith(".at"):
            continue
        category = filename[:-3]  # strip .at
        filepath = os.path.join(src_dir, filename)
        tests = parse_at_file(filepath, category)
        all_tests.extend(tests)
        runtime = sum(1 for t in tests if t["has_runtime_check"])
        with_output = sum(
            1 for t in tests if t["has_runtime_check"] and t["expected_stdout"]
        )
        print(f"  {category}: {len(tests)} tests, {runtime} runtime, {with_output} with expected output")

    # Summary
    total = len(all_tests)
    runtime = sum(1 for t in all_tests if t["has_runtime_check"])
    with_output = sum(
        1 for t in all_tests if t["has_runtime_check"] and t["expected_stdout"]
    )

    print(f"\nTotal: {total} tests, {runtime} runtime, {with_output} with expected output")

    with open(output_file, "w", encoding="utf-8") as f:
        json.dump(all_tests, f, indent=2)

    print(f"Written to {output_file}")


if __name__ == "__main__":
    main()
