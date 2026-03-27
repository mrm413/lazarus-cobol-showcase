# Lazarus C++17 Transpiler — Federal Validation Container

**Torsova LLC** — [lazarus-systems.com](https://lazarus-systems.com)

---

## What This Is

A self-contained Docker environment that compiles and validates all 1,607 C++17 programs transpiled from the GnuCOBOL 3.2 test suite. One command. No dependencies beyond Docker.

## What's In the Container

| Item | Count | Description |
|------|-------|-------------|
| `cpp_output/` | 3,214 files | 1,607 `_clean.cpp` + 1,607 `_hardened.cpp` |
| `cobol_source/` | 1,607 files | Original GnuCOBOL 3.2 `.cbl` test programs |
| `runtime/` | 1 file | 956-line hardening framework header (reference copy) |
| `test_runner.sh` | — | Compilation and validation harness |
| `autofixer.py` | — | Standalone edge-case fixer (3 fix passes, idempotent) |
| Toolchain | — | Ubuntu 24.04, g++ 13+, GnuCOBOL, Python 3 |

## What's NOT In the Container

The Lazarus transpiler engine is proprietary and is not included. This container contains only:
- The **output** of transpilation (C++17 source files)
- The **input** to transpilation (COBOL source files)
- The **runtime library** embedded in each hardened file
- Validation tooling

## Quick Start

From the `lazarus-cobol-showcase/` root:

```bash
docker build -t lazarus-cpp17-validator -f federal-validator/Dockerfile .
docker run --rm lazarus-cpp17-validator
```

Or with docker-compose from `federal-validator/`:

```bash
cd federal-validator
docker-compose up --build
```

## Single Test

```bash
docker run --rm lazarus-cpp17-validator run_fundamental/display_literals
```

## What Gets Validated

For each of the 1,607 test programs:

1. **Hardened C++17 compilation** (primary metric) — `g++ -std=c++17 -Wall -Werror -O2` on the `_hardened.cpp` file. This is the production build with the full safety framework: `FixedString<N>` (buffer overflow protection), `SafeInt<T>` (integer overflow detection), `Decimal` (exact decimal arithmetic), bounds-checked array access, exception handling with COBOL-compatible error codes, and CICS runtime stubs. **All 1,607 hardened files must compile cleanly.**

2. **Clean C++17 compilation** (informational) — `g++ -std=c++17 -Wall -O2` on the `_clean.cpp` file. This is the pre-healer translation — the direct COBOL-to-C++ output before the auto-healer fixes missing declarations. Some clean files have undeclared variables that the hardening stage resolves. Clean compile failures are expected and reported as warnings, not test failures.

3. **Safety audit** — The clean output is scanned for `malloc`, `free`, `new`, `delete`, raw `char*`, and `void*`. These are flagged but not treated as failures.

## The Hardening Layer

Every `_hardened.cpp` file embeds a 947-line framework header. Key components:

| Component | What It Replaces | Protection |
|-----------|-----------------|------------|
| `FixedString<N>` | `char[N]` / `std::string` | Buffer overflow, null terminator bugs |
| `SafeInt<T>` | `int32_t`, `int64_t` | Arithmetic overflow detection |
| `Decimal` | `double` | Exact decimal math (0.1 + 0.2 == 0.3) |
| `PackedDecimal` | raw bytes | COBOL COMP-3 compatible |
| `.at()` access | `[]` subscript | Bounds checking on all arrays |
| `try/catch` in `main()` | none | COBOL-compatible error codes |
| `lazarus::cics` namespace | none | CICS mainframe runtime stubs |
| `safe::` namespace | unchecked I/O | Input validation utilities |

The clean and hardened files produce identical program logic. The hardening adds compile-time and runtime safety without changing behavior.

## Two Output Modes

**Clean** (`_clean.cpp`): Direct translation. Four standard includes (`iostream`, `string`, `cstdlib`, `cmath`). Inline helper functions. No framework dependency. Intended for readability review and logic verification.

**Hardened** (`_hardened.cpp`): Production build. 21 standard includes plus the full `lazarus` namespace. Self-contained — each file embeds the complete framework header. No external dependencies. Intended for deployment.

## Known Issues

31 of the 1,607 programs have no emittable PROCEDURE DIVISION — they are not bugs:

- 28 are `run_manual_screen` tests (COBOL SCREEN SECTION requires an ncurses terminal, no stdout output possible in a headless container)
- 2 are literally empty test programs (no PROCEDURE DIVISION in the COBOL source)
- 1 is a compiler flag test (`configuration/cobc_compiler_flag_on_command_line`)

All 31 still compile successfully. The test runner marks the screen tests and compiler tests as `KNOWN` (not `FAIL`).

The remaining 1,576 programs (98.1%) contain real procedure logic with named C++ functions generated from COBOL paragraphs/sections.

## Test Categories

36 categories covering the full GnuCOBOL 3.2 test suite:

- **Runtime** (`run_*`): Core language, file I/O, functions, Report Writer, extensions, SCREEN
- **Syntax** (`syn_*`): COPY/REPLACE, definitions, literals, subscripts, OCCURS, SET, REDEFINES
- **Data** (`data_*`): Binary, display, packed decimal, pointers
- **Other**: Configuration, listings, used binaries

## Build Requirements

- Docker 20.10+ (or Podman)
- ~2 GB disk for the image
- ~10 minutes for full compilation on a modern machine

## COBOL/C++17 Parity Comparison

The parity harness compiles **both** the original COBOL programs (via GnuCOBOL) and the
transpiled C++17 versions, runs them side by side, and compares stdout output.

```bash
# Quick mode (6 categories, ~5 minutes)
docker run --rm lazarus-cpp17-validator --parity-quick

# Full mode (all categories)
docker run --rm lazarus-cpp17-validator --parity

# Extract the HTML dashboard
docker run --name parity lazarus-cpp17-validator --parity-quick
docker cp parity:/validator/results/parity_report.html ./parity_report.html
docker rm parity
```

Opens in any browser — interactive HTML dashboard with search, filter, and side-by-side
COBOL vs C++17 output for every program.

## GnuCOBOL 3.2 Oracle Parity

The oracle harness compares C++17 output against the **authoritative expected output**
extracted directly from the GnuCOBOL 3.2 test suite `.at` files. No GnuCOBOL compiler
needed — the oracle is the test suite definition itself.

```bash
# Run oracle parity (~1 minute)
docker run --rm lazarus-cpp17-validator --oracle

# Extract the HTML dashboard
docker run --name oracle lazarus-cpp17-validator --oracle
docker cp oracle:/validator/results/oracle_report.html ./oracle_report.html
docker rm oracle
```

**Pipeline:** 36 `.at` files → `parse_testsuite.py` → `testsuite_oracle.json` (1,346 tests,
277 with expected output) → `oracle_harness.sh` → compile C++ → run → compare vs oracle →
interactive HTML report.

## Output Files

After a run, results are in the container at `/validator/results/`:

| File | Contents |
|------|----------|
| `summary.log` | Machine-readable totals (key=value format) |
| `failures.log` | Compiler error details for any failures |
| `safety_audit.log` | Files flagged for raw memory operations |
| `parity_report.html` | Interactive COBOL vs C++17 output dashboard (parity mode) |
| `parity_results.csv` | Machine-readable parity results (parity mode) |
| `oracle_report.html` | Interactive Oracle vs C++17 output dashboard (oracle mode) |
| `oracle/oracle_summary.log` | Machine-readable oracle results (oracle mode) |

To extract results:

```bash
docker run --name val lazarus-cpp17-validator
docker cp val:/validator/results/ ./results/
docker rm val
```

## Autofixer

The `autofixer.py` script fixes three categories of transpiler edge cases automatically:

1. **Orphan braces** — removes stray `}` tokens from incomplete COBOL-to-C++ stub expansions (UDF, TYPEDEF, directive stubs)
2. **Parentheses** — adds explicit `()` around `&&` within `||` to satisfy `-Werror=parentheses`
3. **FixedString cross-size assignment** — patches the embedded header with a `template<M> operator=` for COBOL MOVE semantics across different PIC X(n) sizes

```bash
# Standalone (from repo root)
python3 federal-validator/autofixer.py --dry-run     # report only
python3 federal-validator/autofixer.py                # fix in-place

# Integrated with test runner
AUTOFIX=1 ./test_runner.sh

# Via Docker
docker run --rm lazarus-cpp17-validator --autofix
```

The fixer is idempotent — running it multiple times produces the same result. All fixes are semantics-preserving.

## Reproducing Locally (Without Docker)

On Ubuntu 24.04 with `g++`, `gnucobol`, and `python3` installed:

```bash
cd federal-validator
CPP_DIR=../output COBOL_DIR=../cobol_source ./test_runner.sh
```

---

Built by Torsova LLC. The Lazarus transpilation engine is proprietary software. This validation suite demonstrates the output quality; it does not contain the engine itself.
