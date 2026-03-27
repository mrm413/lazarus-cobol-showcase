# Checkpoint: Phase 1 — Formal Test Suite Run

**Date:** 2026-03-27
**Status:** COMPLETE
**Location:** Local (Windows 11, MSYS2/MinGW64)

## Environment

- **Compiler:** g++ 13.2.0 (MinGW-W64 x86_64-ucrt-posix-seh, built by Brecht Sanders, r8)
- **Clean flags:** `-std=c++17 -Wall -O2`
- **Hardened flags:** `-std=c++17 -Wall -Werror -O2 -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-infinite-recursion`
- **OS:** Windows 11 Home 10.0.22631

## Results (summary.log)

```
date=2026-03-27T06:13:46Z
total=1607
clean_pass=207
hardened_pass=1607
clean_fail=1400
hardened_fail=0
known=9
skipped=0
elapsed_seconds=1731
```

## Key Metrics

| Metric | Value |
|--------|-------|
| Total test programs | 1,607 |
| **Hardened PASS** | **1,607 (100.0%)** |
| Hardened FAIL | 0 |
| Clean PASS | 207 |
| Clean FAIL | 1,400 (expected — pre-autofixer output) |
| Known issues | 9 (SCREEN SECTION + compiler flag tests) |
| Skipped | 0 |
| Wall clock time | 28m 51s |

## failures.log

Empty — zero hardened compilation failures.

## Notes

- Clean compile failures are expected: clean output is the direct transpilation before autofixer patches. The 1,400 clean failures represent files that needed autofixer intervention.
- 9 known issues are SCREEN SECTION tests (require ncurses terminal) and compiler flag tests — all compiled successfully, just flagged as "known" for runtime comparison purposes.
- All 1,607 hardened binaries produced in `results/build/`.
