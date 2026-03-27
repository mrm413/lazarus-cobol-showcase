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

# Parse flags
PARITY_MODE=""
ORACLE_MODE=""
for arg in "$@"; do
    case "$arg" in
        --autofix) export AUTOFIX=1; shift ;;
        --parity)  PARITY_MODE="full"; shift ;;
        --parity-quick) PARITY_MODE="quick"; shift ;;
        --oracle)  ORACLE_MODE=1; shift ;;
    esac
done

# Run the appropriate suite
if [ -n "$ORACLE_MODE" ]; then
    echo "  Running GnuCOBOL 3.2 Oracle Parity Harness..."
    echo ""
    exec ./oracle_harness.sh
elif [ -n "$PARITY_MODE" ]; then
    echo "  Running COBOL/C++17 Parity Harness (mode: $PARITY_MODE)..."
    echo ""
    exec ./parity_harness.sh "$PARITY_MODE"
else
    exec ./test_runner.sh "$@"
fi
