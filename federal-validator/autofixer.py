#!/usr/bin/env python3
"""
Lazarus C++17 Autofixer
Torsova LLC — lazarus-systems.com

Fixes three categories of transpiler edge cases:
  1. Orphan closing braces that prematurely close p_main()
  2. Missing parentheses around && within || (-Werror=parentheses)
  3. Missing FixedString<M> cross-size operator= in embedded headers

Idempotent: safe to run multiple times on the same files.

Usage:
    python autofixer.py [OPTIONS]

Options:
    --root DIR       Repo root (default: auto-detect)
    --dry-run        Report only, don't modify
    --single FILE    Fix one file
    --report PATH    Output report (default: results/autofixer.log)
    --verbose        Print each fix
"""

import os
import re
import sys
import argparse
from pathlib import Path
from datetime import datetime, timezone


# ============================================================================
# CROSS-SIZE OPERATOR TEMPLATE
# ============================================================================

CROSS_SIZE_LINES = [
    "",
    "    // Cross-size assignment (COBOL MOVE semantics: truncate or pad)",
    "    template<std::size_t M>",
    "    FixedString& operator=(const FixedString<M>& other) noexcept {",
    "        data_.fill(' ');",
    "        const std::size_t len = std::min(M, N);",
    "        std::copy_n(other.data(), len, data_.begin());",
    "        return *this;",
    "    }",
]


# ============================================================================
# STRING / COMMENT STRIPPING FOR BRACE COUNTING
# ============================================================================

def strip_literals(line):
    """Remove string/char literals and comments — keeps only structural tokens."""
    out = []
    i = 0
    n = len(line)
    while i < n:
        c = line[i]
        # Line comment
        if c == '/' and i + 1 < n and line[i + 1] == '/':
            break
        # Block comment
        if c == '/' and i + 1 < n and line[i + 1] == '*':
            end = line.find('*/', i + 2)
            i = (end + 2) if end >= 0 else n
            continue
        # String literal
        if c == '"':
            i += 1
            while i < n:
                if line[i] == '\\':
                    i += 2
                    continue
                if line[i] == '"':
                    i += 1
                    break
                i += 1
            continue
        # Char literal
        if c == "'":
            i += 1
            while i < n:
                if line[i] == '\\':
                    i += 2
                    continue
                if line[i] == "'":
                    i += 1
                    break
                i += 1
            continue
        out.append(c)
        i += 1
    return ''.join(out)


def net_braces(line):
    """Net brace count ({=+1, }=-1) ignoring literals and comments."""
    s = strip_literals(line)
    return s.count('{') - s.count('}')


# ============================================================================
# FIXER 1: ORPHAN BRACE REMOVAL
# ============================================================================

_PMAIN_DEF = re.compile(r'void\s+(p_main|P_MAIN)\s*\(\s*\)\s*\{')
_PMAIN_SPLIT = re.compile(r'void\s+(p_main|P_MAIN)\s*\(\s*\)\s*$')
_INT_MAIN = re.compile(r'int\s+main\s*\(')
_BARE_BRACE = re.compile(r'^\s*\}\s*(//.*)?$')
_FUNC_DEF = re.compile(
    r'^(?:static\s+)?(?:inline\s+)?'
    r'(?:void|int|bool|double|float|auto|char|long|short|unsigned)\s+\w+\s*\('
)


def _find_pmain_region(lines):
    """Return (pmain_open_idx, int_main_idx) or (None, None)."""
    pmain = None
    for i, line in enumerate(lines):
        s = line.strip()
        if _PMAIN_DEF.match(s):
            pmain = i
        elif _PMAIN_SPLIT.match(s) and not s.endswith(';'):
            pmain = i
        if _INT_MAIN.match(s) and pmain is not None:
            return pmain, i
    return None, None


def fix_orphan_braces(lines):
    """
    Multi-pass removal of orphan } between p_main() and int main().
    Returns list of (original_line_number, removed_text).
    """
    fixes = []

    for _ in range(10):  # max passes
        pmain_idx, main_idx = _find_pmain_region(lines)
        if pmain_idx is None or main_idx is None:
            break

        # Locate opening brace line
        open_idx = pmain_idx
        if '{' not in strip_literals(lines[pmain_idx]):
            for j in range(pmain_idx + 1, min(pmain_idx + 3, main_idx)):
                if '{' in strip_literals(lines[j]):
                    open_idx = j
                    break

        # Walk depth from opening brace to int main()
        depth = 0
        removed = False

        for i in range(open_idx, main_idx):
            depth += net_braces(lines[i])

            if depth <= 0 and i > open_idx:
                # Check: is there real code (not just comments / bare } /
                # function definitions) afterward?
                has_code = False
                for j in range(i + 1, main_idx):
                    s = lines[j].strip()
                    if not s or s.startswith('//') or s == '}':
                        continue
                    # A function definition after } means p_main closed properly
                    if _FUNC_DEF.match(s):
                        break
                    has_code = True
                    break

                if has_code and _BARE_BRACE.match(lines[i]):
                    # Premature close — remove this orphan }
                    fixes.append((i + 1, lines[i].rstrip()))
                    del lines[i]
                    removed = True
                    break

                if not has_code:
                    # Proper close. Check for stray } at file scope, but
                    # stop before entering any helper function body.
                    for j in range(i + 1, main_idx):
                        s = lines[j].strip()
                        if _FUNC_DEF.match(s):
                            break  # helper function — don't scan its body
                        if _BARE_BRACE.match(lines[j]):
                            fixes.append((j + 1, lines[j].rstrip()))
                            del lines[j]
                            removed = True
                            break
                    break

        if not removed:
            break

    return fixes


# ============================================================================
# FIXER 2: PARENTHESES FOR && WITHIN ||
# ============================================================================

def _top_level_positions(s, token):
    """Find all positions of `token` at paren-depth 0, outside literals."""
    positions = []
    depth = 0
    i = 0
    n = len(s)
    tlen = len(token)
    in_str = False
    sch = None

    while i < n:
        c = s[i]
        if in_str:
            if c == '\\':
                i += 2
                continue
            if c == sch:
                in_str = False
            i += 1
            continue
        if c in ('"', "'"):
            in_str = True
            sch = c
            i += 1
            continue
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
        elif depth == 0 and s[i:i + tlen] == token:
            positions.append(i)
            i += tlen
            continue
        i += 1
    return positions


def _parenthesize_and_groups(cond):
    """Wrap &&-groups in () when mixed with top-level ||. Returns (new, changed)."""
    or_pos = _top_level_positions(cond, ' || ')
    if not or_pos:
        return cond, False

    # Split by top-level ||
    parts = []
    prev = 0
    for p in or_pos:
        parts.append(cond[prev:p])
        prev = p + 4
    parts.append(cond[prev:])

    changed = False
    out = []
    for seg in parts:
        seg = seg.strip()
        if _top_level_positions(seg, ' && '):
            out.append('(' + seg + ')')
            changed = True
        else:
            out.append(seg)

    return ' || '.join(out), changed


def fix_parentheses(lines):
    """Add explicit () around && within || in if-conditions."""
    fixes = []
    for i, line in enumerate(lines):
        if ' && ' not in line or ' || ' not in line:
            continue
        m = re.search(r'\bif\s*\(', line)
        if not m:
            continue

        # Find matching closing )
        paren_start = m.end() - 1
        depth = 0
        j = paren_start
        while j < len(line):
            if line[j] == '(':
                depth += 1
            elif line[j] == ')':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        if depth != 0:
            continue

        condition = line[paren_start + 1:j]
        new_cond, changed = _parenthesize_and_groups(condition)
        if changed:
            new_line = line[:paren_start + 1] + new_cond + line[j:]
            short = condition.strip()[:70]
            fixes.append((i + 1, short))
            lines[i] = new_line

    return fixes


# ============================================================================
# FIXER 3: FIXEDSTRING CROSS-SIZE OPERATOR IN EMBEDDED HEADER
# ============================================================================

def fix_header_crosssize(lines):
    """Insert cross-size template operator= into embedded FixedString class."""
    # Already present?
    for line in lines:
        if 'template<std::size_t M>' in line:
            return []

    # Find string_view operator=
    sv_idx = None
    for i, line in enumerate(lines):
        if 'operator=(std::string_view' in line and 'noexcept' in line:
            sv_idx = i
            break
    if sv_idx is None:
        return []

    # Find its closing } by tracking cumulative depth
    cum = 0
    close_idx = None
    for j in range(sv_idx, min(sv_idx + 12, len(lines))):
        cum += net_braces(lines[j])
        if cum == 0:
            close_idx = j
            break
    if close_idx is None:
        return []

    # Detect line ending from surrounding lines
    le = '\n'
    if close_idx < len(lines) and lines[close_idx].endswith('\r\n'):
        le = '\r\n'

    # Insert after closing }
    ins = close_idx + 1
    for k, text in enumerate(CROSS_SIZE_LINES):
        lines.insert(ins + k, text + le)

    return [(ins + 1, 'cross-size template operator= inserted')]


# ============================================================================
# FILE DISCOVERY AND PROCESSING
# ============================================================================

def find_cpp_files(root, single=None):
    """Collect .cpp files from output/."""
    if single:
        p = Path(single).resolve()
        return [p] if p.exists() else []
    return sorted(Path(root, 'output').rglob('*.cpp'))


def process_file(filepath, dry_run=False):
    """Apply all fixers to one file. Returns list of (type, line, desc)."""
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()

    all_fixes = []
    is_hardened = '_hardened.cpp' in filepath.name

    # Fixer 1 — orphan braces (clean + hardened)
    for lnum, text in fix_orphan_braces(lines):
        all_fixes.append(('brace', lnum, text))

    # Fixer 2 — parentheses (both, but only triggers on -Werror hardened builds)
    for lnum, desc in fix_parentheses(lines):
        all_fixes.append(('parens', lnum, desc))

    # Fixer 3 — header cross-size (hardened only — has embedded header)
    if is_hardened:
        for lnum, desc in fix_header_crosssize(lines):
            all_fixes.append(('header', lnum, desc))

    if all_fixes and not dry_run:
        with open(filepath, 'w', encoding='utf-8', newline='') as f:
            f.writelines(lines)

    return all_fixes


# ============================================================================
# CLI
# ============================================================================

def auto_detect_root():
    """Walk upward from script location / cwd to find output/ dir."""
    candidates = [Path.cwd()]
    try:
        candidates.append(Path(__file__).resolve().parent.parent)
        candidates.append(Path(__file__).resolve().parent)
    except NameError:
        pass
    for c in candidates:
        if (c / 'output').is_dir():
            return c.resolve()
    return None


def main():
    ap = argparse.ArgumentParser(description='Lazarus C++17 Autofixer')
    ap.add_argument('--root', help='Repo root (default: auto-detect)')
    ap.add_argument('--dry-run', action='store_true', help='Report only')
    ap.add_argument('--single', help='Fix one file')
    ap.add_argument('--report', help='Report output path')
    ap.add_argument('--verbose', action='store_true', help='Print each fix')
    args = ap.parse_args()

    root = Path(args.root).resolve() if args.root else auto_detect_root()
    if not root:
        print('ERROR: Cannot locate repo root (no output/ dir). Use --root.',
              file=sys.stderr)
        return 1

    report_path = (Path(args.report) if args.report
                   else root / 'results' / 'autofixer.log')
    report_path.parent.mkdir(parents=True, exist_ok=True)

    files = find_cpp_files(root, args.single)
    mode = 'DRY RUN' if args.dry_run else 'LIVE'

    print(f'\nLazarus C++17 Autofixer [{mode}]')
    print('Torsova LLC \u2014 lazarus-systems.com')
    print('=' * 50)
    print(f'Root:  {root}')
    print(f'Files: {len(files)}')
    print()

    stats = {'files': 0, 'modified': 0, 'brace': 0, 'parens': 0, 'header': 0}
    report_entries = []

    for fpath in files:
        stats['files'] += 1
        fixes = process_file(fpath, dry_run=args.dry_run)

        if fixes:
            stats['modified'] += 1
            try:
                rel = fpath.relative_to(root)
            except ValueError:
                rel = fpath
            report_entries.append((str(rel), fixes))

            for ftype, lnum, desc in fixes:
                stats[ftype] += 1
                if args.verbose:
                    print(f'  [{ftype:6s}] {rel}:{lnum}')

    # ---- Summary ----
    print('=' * 50)
    print(f'  Files scanned:    {stats["files"]}')
    print(f'  Files modified:   {stats["modified"]}')
    print(f'  Brace removals:   {stats["brace"]}')
    print(f'  Paren additions:  {stats["parens"]}')
    print(f'  Header patches:   {stats["header"]}')
    print()

    # ---- Write report ----
    ts = datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write('Lazarus C++17 Autofixer Report\n')
        f.write('=' * 50 + '\n')
        f.write(f'Date: {ts}\n')
        f.write(f'Mode: {mode}\n')
        f.write(f'Root: {root}\n')
        f.write(f'Files scanned: {stats["files"]}\n')
        f.write(f'Files modified: {stats["modified"]}\n')
        f.write(f'Brace removals: {stats["brace"]}\n')
        f.write(f'Paren additions: {stats["parens"]}\n')
        f.write(f'Header patches: {stats["header"]}\n\n')
        for rel, fixes in report_entries:
            f.write(f'{rel}\n')
            for ftype, lnum, desc in fixes:
                f.write(f'  [{ftype}] line {lnum}: {desc}\n')

    print(f'  Report: {report_path}')
    if args.dry_run:
        print('\n  (DRY RUN \u2014 no files modified)')
    print()
    return 0


if __name__ == '__main__':
    sys.exit(main())
