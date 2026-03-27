# Checkpoint: Phase 2 — Docker Container Validation

**Date:** 2026-03-27
**Status:** COMPLETE

## Docker Image

- **Image:** `lazarus-cpp17-validator:latest`
- **Size:** 675 MB
- **Base:** Ubuntu 24.04
- **Toolchain:** g++ 13.3.0, GnuCOBOL 3.1.2.0, Python 3

## Dockerfile Fixes Applied

1. **Symlink:** `RUN ln -s cpp_output output` — autofixer.py searches `<root>/output/` but Dockerfile copies to `cpp_output/`
2. **CRLF fix:** `RUN sed -i 's/\r$//' run.sh test_runner.sh autofixer.py` — Windows line endings break Linux bash

## Container Validation Run

```
date=2026-03-27T14:51:43Z
total=1607
clean_pass=207
hardened_pass=1607
clean_fail=1400
hardened_fail=0
known=9
skipped=0
elapsed_seconds=1280
```

**Result:** ALL HARDENED TESTS PASSED (100.0%)
**Elapsed:** 21m 20s

## --autofix Flag Test

```
docker run --rm lazarus-cpp17-validator --autofix
```

**Result:** ALL HARDENED TESTS PASSED (100.0%)
**Elapsed:** 20m 28s

Autofixer ran inside container, found files already patched (idempotent), then full test suite passed.

## Local vs Container Comparison

| Metric | Local (Windows/MinGW) | Container (Ubuntu 24.04) |
|--------|----------------------|--------------------------|
| Compiler | g++ 13.2.0 | g++ 13.3.0 |
| Total | 1,607 | 1,607 |
| Hardened PASS | 1,607 | 1,607 |
| Hardened FAIL | 0 | 0 |
| Clean PASS | 207 | 207 |
| Known | 9 | 9 |
| Safety flagged | 18 | 17 |
| Elapsed | 28m 51s | 21m 20s |

**Results match.** 100% hardened compilation on both platforms. Minor safety audit difference (18 vs 17) is due to duplicate `syn_definition/typedef_clause` entry in local run (same file flagged twice for both raw memory ops and raw pointers).

## Conclusion

Docker container builds and validates end-to-end on clean Ubuntu 24.04. A federal evaluator can run:
```bash
docker build -t lazarus-cpp17-validator -f federal-validator/Dockerfile .
docker run --rm lazarus-cpp17-validator
```
and see 1,607/1,607 hardened PASS with zero failures.
