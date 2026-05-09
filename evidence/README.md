# Verification Evidence

This folder contains the raw artifacts that back the parity claim in the top-level README. Nothing in the README is asserted that is not derivable from these files.

## Files

| File | What it is |
|------|------------|
| `parity_2026-04-21.json` | Full per-test output from the runtime-parity runner. 1,321 entries — one per federal-suite test. Each entry has `cat`, `test`, `kind`, `status`, `duration`. The top-level summary fields (`global_pass`, `global_tested`, `global_pct`, `global_stats`) are computed from those entries. |
| `cpp17_full_parity.py` | The Python script that produced the parity JSON. 27 KB, 699 lines. The status taxonomy, pass/fail logic, and federal-suite traversal are all visible in source. |
| `compile_validation_2026-05-09.log` | Full output of `federal-validator/test_runner.sh` run inside the Docker container on 2026-05-09. 1,728 lines — banner, per-test PASS/FAIL for all 1,607 tests, then the summary block (Hardened PASS: 1607/1607 = 100%; Clean PASS: 207/1607 = 12.8%; Safety audit: 17 unsafe-pattern flags in clean files). Reproducible via `docker build -t lazarus-cpp17-validator -f federal-validator/Dockerfile . && docker run --rm lazarus-cpp17-validator`. |

## How to Read the JSON

```bash
# Top-line numbers
python -c "import json; d=json.load(open('parity_2026-04-21.json')); print(f'{d[\"global_pass\"]}/{d[\"global_tested\"]} = {d[\"global_pct\"]}%')"
# 1192/1192 = 100.0%

# Status histogram
python -c "import json; d=json.load(open('parity_2026-04-21.json')); [print(f'{k:<22}{v}') for k,v in sorted(d['global_stats'].items(), key=lambda x: -x[1])]"
# BOTH_EMPTY_PASS       553
# MATCH                 300
# COMPILE_FAIL_PASS     272
# SCREEN_MATCH          67
# NO_EXE                48
# EXPECTED_SKIP         41
# NO_GOLDEN             39
# SCREEN_NO_GOLDEN      1

# Per-category pass rate
python -c "import json; d=json.load(open('parity_2026-04-21.json')); [print(f'{c[\"cat\"]:<25}{c[\"passed\"]}/{c[\"tested\"]} = {c[\"pct\"]}%') for c in d['per_category']]"

# Find any failures (should be empty)
python -c "import json; d=json.load(open('parity_2026-04-21.json')); fails=[r for r in d['results'] if r['status'] in ('VALUE_DIFF','COMPILE_FAIL_MISMATCH','TIMEOUT','ERROR')]; print('Failures:', len(fails)); [print(' ',f['cat'],f['test'],f['status']) for f in fails]"
# Failures: 0
```

## Status Taxonomy (from the runner)

The runner classifies each test into a `kind` based on what artifacts the federal-suite test directory contains, then runs the appropriate check and assigns a `status`:

| Kind | What's in the test dir | Pass status | Fail statuses |
|------|------------------------|-------------|---------------|
| `MATCH` | `prog.exe` + non-empty `stdout.txt` | `MATCH` (stdout matches byte-for-byte) | `VALUE_DIFF`, `TIMEOUT`, `ERROR` |
| `BOTH_EMPTY` | `prog.exe` + empty `stdout.txt` | `BOTH_EMPTY_PASS` (Lazarus also produces empty stdout) | `VALUE_DIFF` (Lazarus produced output where reference produced nothing) |
| `COMPILE_FAIL` | `prog.cob` only — no `prog.exe` (reference compiler rejected the source) | `COMPILE_FAIL_PASS` (Lazarus also rejected the invalid source — correct negative-test behavior) | `COMPILE_FAIL_MISMATCH` (Lazarus accepted invalid COBOL — bug) |
| `SCREEN` (run_manual_screen) | Terminal-emulator capture available | `SCREEN_MATCH` (capture matches reference) | screen diff |
| `NO_GOLDEN` | No reference output | `NO_GOLDEN` (recorded but uncomparable) | — |
| `NO_EXE` | No reference binary | `NO_EXE` (cannot run side-by-side) | — |

`EXPECTED_SKIP` is set when the test triggers a pre-documented 32-bit-vs-64-bit GnuCOBOL behavioral difference (debug column offsets, exception report format, etc.) — see the `EXPECTED_SKIPS.md` list in the source pipeline. These are skips, not passes.

## Reproducing the Run

The runner needs the GnuCOBOL 3.2 federal test suite (1,321 test directories, each containing `prog.cob`, optional `prog.exe`, and optional `stdout.txt`/screen capture). The path is configured at the top of `cpp17_full_parity.py`. Once the path is set:

```bash
python cpp17_full_parity.py             # all categories
python cpp17_full_parity.py --cat run_misc   # one category
```

Expected runtime: ~7-8 minutes for the full suite (~456 seconds in the recorded run).

## What Counts as Passing

The runner's `is_pass()` function (line 504) defines passing as:

```python
status in ("MATCH", "BOTH_EMPTY_PASS", "SCREEN_MATCH", "COMPILE_FAIL_PASS")
```

`global_tested` excludes the four non-checkable statuses (`EXPECTED_SKIP`, `NO_GOLDEN`, `NO_EXE`, `SCREEN_NO_GOLDEN`). The 1,192/1,192 number reflects passing within the runtime-checkable subset only. The 129 non-checkable tests are listed in the JSON with their reasons but are not counted toward pass or fail.
