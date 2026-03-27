# Lazarus C++17 — Mainframe Parity Certification

**Date:** 2026-03-27
**Vendor:** Torsova LLC — lazarus-systems.com
**Status:** 12/12 TIERS COMPLETE

## Executive Summary

The Lazarus C++17 mainframe platform achieves full ecosystem coverage across 12 tiers,
matching the scope of the Ironclad Rust target. All 1,734 tests pass with zero failures.
The platform comprises 59 header-only C++17 libraries totaling 30,292 lines of code,
plus 17,612 lines of test code across 12 test suites.

Every file compiles clean under `g++ -std=c++17 -Wall -Werror -Wextra`.

## Test Results

| Tier | Component | Tests | Assertions | Status |
|------|-----------|-------|------------|--------|
| 1 | JCL Transpiler | 136 | 302 | PASS |
| 2 | DFSORT/SYNCSORT | 148 | 1,281 | PASS |
| 3 | DB2 Schema Migration | 185 | 515 | PASS |
| 4 | GDG + Flat File | 102 | — | PASS |
| 5 | PL/I Transpiler | 267 | — | PASS |
| 6 | REXX Transpiler | 223 | — | PASS |
| 7 | Easytrieve | 151 | 407 | PASS |
| 8 | RACF Importer | 102 | 268 | PASS |
| 9 | Job Scheduler | 102 | — | PASS |
| 10 | Parity Framework | 130 | 334 | PASS |
| 11 | Unified Pipeline | 158 | 315 | PASS |
| 12 | Final Certification | 30 | 153 | PASS |
| **TOTAL** | **12 tiers** | **1,734** | **5,095+** | **ALL PASS** |

## Platform Statistics

| Metric | Count |
|--------|-------|
| Header-only libraries | 59 |
| Source lines (headers) | 30,292 |
| Test files | 12 |
| Test lines | 17,612 |
| Total lines | 47,904 |
| Compile flags | -std=c++17 -Wall -Werror -Wextra |
| Compiler | g++ 13.2.0 (MinGW-W64) |
| Test framework | doctest 2.4.11 |
| External dependencies | ZERO (air-gapped) |

## Architecture

All libraries are header-only under `include/lazarus/`:

```
include/lazarus/
  jcl/        — lexer, parser, proc_resolver, translator, shell_translator, utilities
  sort/       — parser, engine, icetool, ebcdic, translator
  db2/        — ddl_parser, postgres_translator, copybook_mapper, ebcdic, record_reader
  file/       — gdg, sequential, vsam, pds, dsn
  pli/        — lexer, parser, ast, builtins, runtime, translator, preprocessor
  rexx/       — lexer, parser, ast, builtins, python_translator, runtime
  easytrieve/ — lexer, parser, ast, translator, runtime
  racf/       — parser, profile, policy_emitter, audit
  scheduler/  — schedule_model, tws_parser, ca7_parser, airflow_emitter, cron_emitter
  parity/     — runner, comparator, audit, regression, report
  pipeline/   — detector, router, graph, assessment, orchestrator, exporter
```

## C++17 Features Used

- std::variant for type-safe tagged unions (AST nodes, file handlers, conditions)
- std::optional for nullable fields
- std::string_view for zero-copy parsing
- if constexpr for compile-time branching
- constexpr for compile-time lookup tables (EBCDIC, type mappings)
- Structured bindings for multi-return
- Fold expressions for variadic operations
- RAII for all resource management
- std::filesystem for portable file operations
- Template metaprogramming for FixedString<N>, PackedDecimal<P,S>
