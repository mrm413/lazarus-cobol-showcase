#!/usr/bin/env bash
# ============================================================================
# Lazarus C++17 Transpiler — Federal Validation Runner
# Torsova LLC — lazarus-systems.com
# ============================================================================

set -euo pipefail

cat <<'BANNER'

  ╔══════════════════════════════════════════════════════════╗
  ║                                                          ║
  ║   LAZARUS C++17 TRANSPILER                               ║
  ║   Federal Validation Suite                               ║
  ║                                                          ║
  ║   1,607 GnuCOBOL 3.2 Test Programs                      ║
  ║   3,214 C++17 Compilations (clean + hardened)            ║
  ║                                                          ║
  ║   Torsova LLC — lazarus-systems.com                      ║
  ║                                                          ║
  ╚══════════════════════════════════════════════════════════╝

BANNER

# Verify directory structure
for dir in cpp_output cobol_source; do
    if [ ! -d "$dir" ]; then
        echo "ERROR: Missing directory: $dir"
        echo "Make sure the container has access to transpiler output."
        exit 1
    fi
done

# Count files
CPP_COUNT=$(find cpp_output -name "*.cpp" | wc -l)
CBL_COUNT=$(find cobol_source -name "*.cbl" | wc -l)

echo "  Found $CPP_COUNT C++ files, $CBL_COUNT COBOL sources"
echo ""

# Check for --autofix flag
for arg in "$@"; do
    if [ "$arg" = "--autofix" ]; then
        export AUTOFIX=1
        shift
        break
    fi
done

# Run the test suite
exec ./test_runner.sh "$@"
