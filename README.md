# Lazarus: COBOL-to-Hardened C++17 Transpiler Output

**Production output (hardened C++17): 1,607 / 1,607 compile (100%)** under `g++ -std=c++17 -Wall -Werror -O2`. Validated 2026-05-09 in the `federal-validator/` Docker container.

**Runtime parity (federal suite): 1,192 / 1,192 (100%)** against GnuCOBOL 3.2, verified 2026-04-21.

This repository contains the **output** of the Lazarus transpilation system — not the system itself. Every `.cpp` file here was generated automatically from legacy COBOL source code and compiled as production-grade, security-hardened C++17. Lazarus is a proprietary transpilation engine built by [Torsova LLC](https://lazarus-systems.com).

---

## At a Glance

| Metric | Value |
|--------|-------|
| Runtime parity (federal suite) | **1,192 / 1,192 (100.0%)** |
| Compile rate (hardened, production) | **1,607 / 1,607 (100.0%)** |
| Compile rate (clean, pre-framework) | 207 / 1,607 (12.8%) — see [note below](#a-note-on-clean-vs-hardened) |
| Test corpus size | 1,321 tests across 36 categories |
| Programs with procedure logic | 1,576 (98.1%) |
| Programs without procedure logic | 31 (SCREEN-only, empty stubs, compiler-flag tests) |
| Total C++17 lines (clean output) | 85,724 |
| Pipeline speed | ~1.5 seconds per program |
| Last runtime-parity verification | 2026-04-21 11:31:30 UTC |
| Last compile-rate verification | 2026-05-09 (federal-validator Docker, 20m10s) |
| Runtime-parity runner | `evidence/cpp17_full_parity.py` |
| Raw runtime-parity evidence | `evidence/parity_2026-04-21.json` |
| Raw compile-rate evidence | `evidence/compile_validation_2026-05-09.log` |
| Compiler | g++ 13.2.0 (MinGW-W64 ucrt) host, g++ 13.3.0 (Ubuntu 24.04) container |

---

## A Note on Clean vs. Hardened

Each transpiled program ships in two forms:

- **`{name}_hardened.cpp`** — production output. Wrapped in the 947-line safety framework (`FixedString<N>`, `SafeInt<T>`, `Decimal`, `PackedDecimal`, RAII, COBOL-coded try/catch). 1,607 / 1,607 compile clean under `-std=c++17 -Wall -Werror -O2`. **This is what production uses.**
- **`{name}_clean.cpp`** — direct translation, no framework. Useful for code review of the raw COBOL→C++ logic. 207 / 1,607 compile out-of-the-box.

The 1,400 clean files that don't compile reveal known transpiler emit gaps in the clean variant — undeclared `RETURN_CODE`, occasional brace-balance edge cases, missing `FixedString<M>` cross-size operator overloads. The hardening framework provides those definitions itself, so the hardened build is unaffected. The repo also ships [`federal-validator/autofixer.py`](federal-validator/autofixer.py), a 3-rule deterministic patcher that closes those specific compile gaps in the clean variant when needed.

**Important:** The autofixer fixes *compile errors only*. It does not address security. Vulnerabilities (raw memory, unbounded strings, integer overflow) are addressed by the **hardening stage** (`[H]` in the pipeline below) — a separate, mandatory pipeline step that replaces unsafe patterns. The validator's safety audit greps the *clean* variant for raw memory ops (`malloc`/`free`/`new`/`delete`) and raw pointers (`char *`/`void *`); the most recent run flagged 17 such patterns in clean files. Those patterns do not appear in the hardened files because the hardener replaces them.

---

## What "1,192 / 1,192" Actually Means

The federal test suite contains 1,321 tests. Of these, **1,192 are runtime-checkable** (they have either a golden stdout file, a screen capture, or a known-invalid program that the reference compiler rejects). All 1,192 pass. The remaining 129 are non-checkable for reasons documented below — they are not failures, they are tests we cannot verify side-by-side under the current environment.

This is the full breakdown straight out of the runner:

| Status | Count | What it means |
|--------|------:|---------------|
| `MATCH` | 300 | Lazarus C++17 stdout matches GnuCOBOL stdout, byte-for-byte |
| `BOTH_EMPTY_PASS` | 553 | Reference produces empty stdout; Lazarus also produces empty stdout. Exit code irrelevant by convention |
| `COMPILE_FAIL_PASS` | 272 | Test is intentionally invalid COBOL; reference rejects it; Lazarus correctly rejects it too (negative test) |
| `SCREEN_MATCH` | 67 | Terminal-emulator capture matches reference screen output |
| **Total passing** | **1,192** | |
| `EXPECTED_SKIP` | 41 | Documented 32-bit/64-bit GnuCOBOL behavioral differences (debug column offsets, exception format, etc.) |
| `NO_GOLDEN` | 39 | No reference output exists; nothing to compare against |
| `NO_EXE` | 48 | No pre-built reference binary; can't run side-by-side |
| `SCREEN_NO_GOLDEN` | 1 | Screen test with no reference capture |
| **Total non-checkable** | **129** | |
| **Test corpus total** | **1,321** | |
| **Failures** | **0** | |

`BOTH_EMPTY_PASS` and `COMPILE_FAIL_PASS` are deliberately broken out so they cannot be conflated with full output match. Each is a distinct, valid pass condition with explicit semantics in the runner.

---

## Reproducing the Result

Three artifacts make this auditable:

1. **`evidence/parity_2026-04-21.json`** — the runner's full per-test output, machine-readable. 1,321 entries; sum of every status type matches the table above.
2. **`evidence/cpp17_full_parity.py`** — the actual runner script. Read it: there is no number in this README that the runner does not produce verbatim.
3. **`output/`** — every transpiled C++17 program (clean + hardened) that the runner exercised.

```bash
# Verify the JSON matches the README claims
python -c "import json; d=json.load(open('evidence/parity_2026-04-21.json')); print(d['global_pass'], '/', d['global_tested'], '=', d['global_pct'], '%')"
# Output: 1192 / 1192 = 100.0 %

# Inspect the breakdown
python -c "import json; d=json.load(open('evidence/parity_2026-04-21.json')); [print(f'{k}: {v}') for k,v in d['global_stats'].items()]"
```

The runner depends on the GnuCOBOL 3.2 federal test suite (~1,321 tests with `prog.cob`, `prog.exe`, and `stdout.txt` per category). Path is configured in the script.

---

## What's in This Repo

```
lazarus-cobol-showcase/
  README.md                          # This file
  evidence/
    parity_2026-04-21.json           # Full per-test runner output (raw evidence)
    cpp17_full_parity.py             # The runner script that produced it
    README.md                        # How to read the evidence
  docs/
    FINAL_REPORT_100_PERCENT.md      # Earlier 1,607/1,607 compile-rate report
  checkpoints/                       # Tiered milestones (12 tiers, mainframe ecosystem coverage)
  federal-validator/                 # Docker-based compile validator
    Dockerfile
    test_runner.sh
  cobol_source/                      # All 1,607 original COBOL programs
  output/                            # All 1,607 transpiled C++17 programs
    {category}/{test}/
      {test}_clean.cpp               # Direct C++17 translation (pre-hardening)
      {test}_hardened.cpp            # 947-line safety framework wrapped (production)
  samples/                           # Curated before/after examples
```

---

## Example: COBOL Input → Lazarus C++17

### COBOL Input

```cobol
       IDENTIFICATION DIVISION.
       PROGRAM-ID. prog.
       PROCEDURE DIVISION.
           DISPLAY "BEFORE FINAL - SHOULD DISPLAY"
           STOP RUN.
```

### Clean C++17 Output

```cpp
#include <iostream>
#include <string>

int RETURN_CODE = 0;

void P_MAIN() {
    std::cout << "BEFORE FINAL - SHOULD DISPLAY" << std::endl;
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}
```

### Hardened C++17 Output (abbreviated)

```cpp
// 947-line safety framework header:
//   FixedString<N>, SafeInt<T>, Decimal, PackedDecimal,
//   CICS runtime stubs, COBOL-compatible error codes

int RETURN_CODE = 0;

void p_main() {
    std::cout << "BEFORE FINAL - SHOULD DISPLAY" << std::endl;
    return;
}

int main() {
    try {
        p_main();
        return RETURN_CODE;
    } catch (const std::out_of_range& e) {
        std::cerr << "[LAZARUS ERROR] Bounds violation: " << e.what() << std::endl;
        return 281;
    } catch (const std::overflow_error& e) {
        std::cerr << "[LAZARUS ERROR] Numeric overflow: " << e.what() << std::endl;
        return 284;
    } catch (...) {
        std::cerr << "[LAZARUS ERROR] Unknown error" << std::endl;
        return 999;
    }
}
```

Compile:
```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -o output source.cpp
```

---

## The Six-Stage Pipeline

```
  COBOL Source
      |
      v
  [D] Dialect Detection ---- COBOL-74 / COBOL-85 / COBOL-2002 / MF / IBM extensions
      |
      v
  [P] Preprocessing -------- COPY books, continuation lines, compiler directives
      |
      v
  [T] Transpilation -------- COBOL -> intermediate representation
      |
      v
  [X] Transformation ------- Paragraphs, sections, DECLARATIVES; conditions, PERFORM,
      |                       file I/O, WORKING-STORAGE, EVALUATE, STRING resolution
      |
      v
  [H] Hardening ------------ FixedString, SafeInt, Decimal, bounds checking,
      |                       exception handling, CICS stubs
      |
      v
  [C] Compilation ---------- g++ -std=c++17 with auto-healing for known edge cases
      |
      v
  Hardened C++17 Executable
```

Every stage has a pass/fail gate. The auto-healer at the compile stage fixes mechanical issues (missing forward declarations, type mismatches) without changing program semantics.

---

## Security Hardening

C++17 is not memory-safe by default. Lazarus compensates with a dedicated hardening stage:

| COBOL Type | Raw C++ (Unsafe) | Lazarus C++17 (Hardened) |
|------------|-----------------|-------------------------|
| `PIC X(30)` | `char name[30]` | `FixedString<30> name` |
| `PIC S9(9) COMP` | `int32_t amount` | `SafeInt<int32_t> amount` |
| `PIC S9(7)V9(2)` | `double balance` | `Decimal balance(18, 2)` |
| `COMP-3` | raw byte packing | `PackedDecimal` |
| Array access | `arr[i]` | `arr.at(i)` (bounds-checked) |
| String copy | `strcpy(dst, src)` | `dst = src` (auto-truncated) |
| Program entry | `main()` | `try/catch` with COBOL-compatible error codes |

Hardened output uses RAII exclusively — no raw `new`/`delete`, no `malloc`/`free`, no raw pointers.

### C++17 vs. Rust: An Honest Assessment

Rust is inherently memory-safe. The borrow checker makes buffer overflows and use-after-free impossible at compile time. Lazarus produces hardened C++17 instead because:
- Most enterprises migrating off COBOL already have C++ infrastructure and teams.
- C++17 links natively with C libraries, OS APIs, and mainframe middleware.
- Regulated industries have established C++ audit and review processes.
- The available C++ workforce is significantly larger than Rust.

The day-one safety profile of Lazarus C++17 output matches Rust for the vulnerability classes that matter most (buffer overflows, integer overflows, use-after-free via RAII). Day-two maintenance is where they diverge — Rust enforces safety on new code via the compiler; C++ relies on the developer to keep using the safety framework.

---

## Test Categories

36 categories covering the full GnuCOBOL 3.2 federal test suite:

| Category | Programs | Description |
|----------|---------:|-------------|
| `configuration` | 15 | Compiler flags, dialect settings, source formats |
| `data_binary` | 11 | COMP, COMP-4, binary data items |
| `data_display` | 11 | DISPLAY format numeric/alphanumeric |
| `data_packed` | 25 | COMP-3 packed decimal |
| `data_pointer` | 1 | Pointer and address operations |
| `listings` | 54 | Source listings, REPLACE, symbol tables |
| `run_accept` | 6 | ACCEPT statement |
| `run_extensions` | 140 | MF/IBM extensions, system routines |
| `run_file` | 132 | Sequential, indexed, relative file I/O |
| `run_functions` | 129 | Intrinsic functions |
| `run_fundamental` | 123 | Core language: MOVE, ADD, IF, PERFORM, CALL, STRING |
| `run_initialize` | 16 | INITIALIZE statement |
| `run_manual_screen` | 64 | SCREEN SECTION (terminal I/O) |
| `run_misc` | 244 | SORT, MERGE, INSPECT, EXIT, reference modification |
| `run_ml` | 14 | JSON/XML GENERATE |
| `run_refmod` | 17 | Reference modification |
| `run_reportwriter` | 29 | Report Writer (RD, GENERATE, TERMINATE) |
| `run_returncode` | 5 | RETURN-CODE and STOP RUN |
| `run_subscripts` | 17 | Table subscripts and indexing |
| `syn_copy` | 37 | COPY and REPLACE directives |
| `syn_definition` | 83 | Data definition validation |
| `syn_file` | 53 | File control validation |
| `syn_functions` | 18 | Function syntax validation |
| `syn_literals` | 21 | Numeric and string literals |
| `syn_misc` | 195 | Miscellaneous syntax validation |
| `syn_move` | 18 | MOVE statement validation |
| `syn_multiply` | 3 | MULTIPLY statement validation |
| `syn_occurs` | 22 | OCCURS clause validation |
| `syn_redefines` | 17 | REDEFINES clause validation |
| `syn_refmod` | 5 | Reference modification syntax |
| `syn_reportwriter` | 12 | Report Writer syntax |
| `syn_screen` | 21 | Screen section syntax |
| `syn_set` | 2 | SET statement validation |
| `syn_subscripts` | 6 | Subscript syntax |
| `syn_value` | 13 | VALUE clause validation |
| `used_binaries` | 28 | Binary/executable linkage |
| **Total** | **1,607** | |

---

## Federal Validation Container

The `federal-validator/` directory contains a self-contained Docker environment that compiles all 1,607 programs:

```bash
docker build -t lazarus-cpp17-validator -f federal-validator/Dockerfile .
docker run --rm lazarus-cpp17-validator
```

See [`federal-validator/README-EVALUATOR.md`](federal-validator/README-EVALUATOR.md) for full documentation.

---

## Sister Pipelines

Lazarus is one of three COBOL transpilers in the same suite. Each targets a different output language with its own scope, harness, and pass criteria:

- **[Ironclad — COBOL → Rust](https://github.com/mrm413/cms-medicare-ironclad-showcase)** — production CMS Medicare programs, byte-for-byte parity validation
- **Bobo — COBOL → Java** — parity validation in progress
- **Lazarus — COBOL → C++17** — *this pipeline; 1,192/1,192 runtime parity on federal suite*

The C++17 runtime parity number (1,192) is the largest of the three because the federal-suite harness is the most permissive (it counts negative-test rejection, empty-stdout match, and screen-capture match as separate, explicitly-labeled categories). The Rust and Java pipelines run different harnesses against different scopes; they are not directly comparable as raw numbers, only as methodologies.

---

## Also Available

- [CMS Medicare — Lazarus C++17](https://github.com/mrm413/cms-medicare-lazarus-showcase) — 55 production CMS Medicare pricer programs (92,535 LOC) transpiled and compiled (100%)
- [Lazarus CardDemo](https://github.com/mrm413/lazarus-carddemo-showcase) — AWS CardDemo CICS/COBOL system, 44 programs + CICS runtime + 3270 web UI

---

## Built By

**Torsova LLC** — [lazarus-systems.com](https://lazarus-systems.com)

Lazarus is part of a suite of legacy modernization tools including transpilers for COBOL (C++17 and Rust), VB6, Stored Procedures, Crystal Reports, SAS, and Microsoft Access.

---

## License

Licensed under the [Apache License, Version 2.0](LICENSE).

The original GnuCOBOL test programs are from the [GnuCOBOL project](https://gnucobol.sourceforge.io/).

All modifications and additions — including the C++17 transpiled programs, security hardening, build system, test harness, and verification artifacts — are Copyright 2025–2026 Michael R. Mull / Lazarus Systems. See [NOTICE](NOTICE) for details.
