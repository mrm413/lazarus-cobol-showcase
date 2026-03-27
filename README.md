# Lazarus: COBOL-to-Hardened C++17 Transpiler Output

**1,607 / 1,607 compiled (100%)** against the GnuCOBOL 3.2 test suite.

This repository contains the **output** of the Lazarus transpilation system -- not the system itself. Every file here was generated automatically from legacy COBOL source code and compiled as production-grade, security-hardened C++17.

Lazarus is a proprietary transpilation engine built by [Torsova LLC](https://lazarus-systems.com). The source code for Lazarus is not included in this repository.

---

## At a Glance

| Metric | Value |
|--------|-------|
| COBOL programs transpiled | 1,607 |
| Programs with procedure logic | 1,576 (98.1%) |
| Programs without procedure logic | 31 (SCREEN SECTION, empty stubs, compiler tests) |
| Compile success rate | 100.0% |
| Test categories | 36 |
| Total C++17 lines (clean output) | 85,724 |
| Pipeline speed | ~1.5 seconds per program |
| C++ standard | C++17 (`-std=c++17 -Wall -Wextra -Wpedantic`) |

The 31 programs without procedure logic are not failures. 28 are COBOL SCREEN SECTION tests that use terminal-based I/O with no emittable procedure body. The remaining 3 are empty test stubs and a compiler flag validation test. All 31 still compile and produce executables.

---

## What Is This?

Lazarus takes legacy COBOL programs and produces modern, safe, compilable C++17. This repository demonstrates that capability at scale: every test program in the GnuCOBOL 3.2 validation suite has been transpiled, hardened, and compiled to a working executable.

Each test produces two output files:

- **`_clean.cpp`** -- Direct C++17 translation of the COBOL logic. Minimal includes, readable, no framework dependency. This is the pre-hardening snapshot showing the raw translation.
- **`_hardened.cpp`** -- Production build. Same logic wrapped in a 947-line safety framework with bounds checking, overflow protection, type safety, CICS stubs, and exception handling. Self-contained -- each file embeds the full framework.

### Example: COBOL Input

```cobol
       IDENTIFICATION DIVISION.
       PROGRAM-ID. prog.
       PROCEDURE DIVISION.
           DISPLAY "BEFORE FINAL - SHOULD DISPLAY"
           STOP RUN.
```

### Example: Clean C++17 Output

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

### Example: Hardened C++17 Output (abbreviated)

```cpp
// 947-line safety framework header:
// - FixedString<N>: bounds-checked fixed-length strings
// - SafeInt<T>: overflow-checked integers
// - Decimal: exact decimal arithmetic
// - PackedDecimal: COBOL COMP-3 compatible
// - CICS runtime stubs
// - Error codes matching COBOL return codes

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

Every hardened file compiles cleanly with:
```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -o output source.cpp
```

---

## Repository Structure

```
lazarus-cobol-showcase/
  README.md
  docs/
    FINAL_REPORT_100_PERCENT.md    # Detailed test results
  federal-validator/               # Docker-based validation container
    Dockerfile
    docker-compose.yml
    test_runner.sh                 # Compiles all 3,214 C++ files
    README-EVALUATOR.md
  samples/                         # Curated before/after examples
  cobol_source/                    # All 1,607 original COBOL programs
    configuration/
    data_binary/
    ...
  output/                          # All 1,607 transpiled C++17 programs
    configuration/
      test_name/
        test_name_clean.cpp        # Direct C++17 translation
        test_name_hardened.cpp     # Security-hardened production build
    data_binary/
    ...
```

---

## Federal Validation Container

The `federal-validator/` directory contains a self-contained Docker environment that compiles and validates all 1,607 programs. One command:

```bash
docker build -t lazarus-cpp17-validator -f federal-validator/Dockerfile .
docker run --rm lazarus-cpp17-validator
```

See [`federal-validator/README-EVALUATOR.md`](federal-validator/README-EVALUATOR.md) for full documentation.

---

## The Six-Stage Pipeline

```
  COBOL Source
      |
      v
  [D] Dialect Detection ---- Identifies COBOL-74, COBOL-85, COBOL-2002, MF extensions
      |
      v
  [P] Preprocessing -------- Resolves COPY books, continuation lines, compiler directives
      |
      v
  [T] Transpilation -------- Compiles COBOL to intermediate representation
      |
      v
  [X] Transformation ------- Extracts paragraphs, sections, DECLARATIVES
      |                       Translates conditions, PERFORM loops, file I/O
      |                       Resolves variables, WORKING-STORAGE, EVALUATE, STRING
      |
      v
  [H] Hardening ------------ Applies security framework (FixedString, SafeInt, Decimal)
      |                       Adds bounds checking, exception handling, CICS stubs
      |
      v
  [C] Compilation ---------- Compiles with g++ -std=c++17 and auto-heals edge cases
      |
      v
  Hardened C++17 Executable
```

Every stage has a pass/fail gate. The auto-healer at the compile stage fixes common issues (missing declarations, type mismatches) without changing program semantics.

---

## Security Hardening

C++17 is not inherently memory-safe. Lazarus compensates with a dedicated hardening stage:

| COBOL Type | Raw C++ (Unsafe) | Lazarus C++17 (Hardened) |
|------------|-----------------|-------------------------|
| `PIC X(30)` | `char name[30]` | `FixedString<30> name` |
| `PIC S9(9) COMP` | `int32_t amount` | `SafeInt<int32_t> amount` |
| `PIC S9(7)V9(2)` | `double balance` | `Decimal balance(18, 2)` |
| `COMP-3` | raw byte packing | `PackedDecimal` |
| Array access | `arr[i]` | `arr.at(i)` (bounds-checked) |
| String copy | `strcpy(dst, src)` | `dst = src` (auto-truncated) |
| Program entry | `main()` | `try/catch` with COBOL-compatible error codes |

The hardened output uses RAII exclusively -- no raw `new`/`delete`, no `malloc`/`free`, no raw pointers.

### C++17 vs. Rust: An Honest Assessment

Rust is inherently memory-safe. The borrow checker makes buffer overflows and use-after-free impossible at compile time. There is no "hardening stage" in a Rust transpiler because the language itself is the hardening.

Lazarus produces hardened C++17 instead because:
- Most enterprises migrating off COBOL already have C++ infrastructure and teams
- C++17 links natively with C libraries, OS APIs, and mainframe middleware
- Regulated industries have established C++ audit and review processes
- The available C++ workforce is significantly larger than Rust

The day-one safety profile of Lazarus C++17 output matches Rust for the vulnerability classes that matter most (buffer overflows, integer overflows, use-after-free via RAII). Day-two maintenance is where they diverge -- Rust enforces safety on new code via the compiler; C++ relies on the developer to keep using the safety framework.

---

## Test Categories

36 categories covering the full GnuCOBOL 3.2 test suite:

| Category | Tests | Description |
|----------|-------|-------------|
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

---

## Also Available

Lazarus also transpiles production COBOL. All 55 CMS Medicare payment system programs (92,535 lines) have been transpiled to hardened C++17 with 100% compile success:

- [CMS Medicare -- Lazarus C++17](https://github.com/mrm413/cms-medicare-lazarus-showcase) -- 55 programs, 97,924 lines of hardened C++17
- [CMS Medicare -- Ironclad Rust](https://github.com/mrm413/cms-medicare-ironclad-showcase) -- 55 programs, 169,475 lines of Rust
- [Lazarus CardDemo Showcase](https://github.com/mrm413/lazarus-carddemo-showcase) -- 44 AWS CardDemo CICS/COBOL programs transpiled to C++17 (100%)

---

## Built By

**Torsova LLC** -- [lazarus-systems.com](https://lazarus-systems.com)

Lazarus is part of a suite of legacy modernization tools including transpilers for COBOL (C++17 and Rust), VB6, Stored Procedures, Crystal Reports, SAS, and Microsoft Access.

---

## License

Licensed under the [Apache License, Version 2.0](LICENSE).

The original GnuCOBOL test programs are from the [GnuCOBOL project](https://gnucobol.sourceforge.io/).

All modifications and additions -- including the C++17 transpiled programs, security hardening, build system, and test harness -- are Copyright 2025 Michael R. Mull / Lazarus Systems. See [NOTICE](NOTICE) for details.
