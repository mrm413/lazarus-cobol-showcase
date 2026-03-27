#!/usr/bin/env bash
# ============================================================================
# Lazarus C++17 Transpiler — Runtime Smoke Test
# Torsova LLC — lazarus-systems.com
#
# Runs all compiled hardened binaries with a timeout to verify they don't
# crash, segfault, or hang. Compilation proves syntax; runtime proves
# structural soundness.
# ============================================================================

set -euo pipefail

RESULTS_DIR="${RESULTS_DIR:-./results}"
BUILD_DIR="${RESULTS_DIR}/build"
TIMEOUT_SEC="${TIMEOUT_SEC:-5}"
SMOKE_LOG="${RESULTS_DIR}/runtime_smoke.log"

if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build directory not found: $BUILD_DIR"
    echo "Run test_runner.sh first to compile binaries."
    exit 1
fi

# ---------------------------------------------------------------------------
# Colors
# ---------------------------------------------------------------------------

if [ -t 1 ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    YELLOW='\033[1;33m'
    CYAN='\033[0;36m'
    BOLD='\033[1m'
    RESET='\033[0m'
else
    GREEN='' RED='' YELLOW='' CYAN='' BOLD='' RESET=''
fi

# ---------------------------------------------------------------------------
# SCREEN SECTION tests — skip (require ncurses terminal)
# ---------------------------------------------------------------------------

SKIP_CATEGORY="run_manual_screen"

# ---------------------------------------------------------------------------
# Counters
# ---------------------------------------------------------------------------

TOTAL=0
EXIT_0=0
EXIT_NONZERO=0
CRASHED=0
TIMEDOUT=0
SKIPPED=0

> "$SMOKE_LOG"

NONZERO_LIST=""
CRASH_LIST=""
TIMEOUT_LIST=""

printf "\n${BOLD}Lazarus C++17 Runtime Smoke Test${RESET}\n"
printf "Torsova LLC — lazarus-systems.com\n"
printf "=====================================\n\n"
printf "Timeout:   %d seconds per binary\n" "$TIMEOUT_SEC"
printf "Build dir: %s\n\n" "$BUILD_DIR"

START_TIME=$(date +%s)

# ---------------------------------------------------------------------------
# Find and run all hardened binaries
# ---------------------------------------------------------------------------

while IFS= read -r binary; do
    # Extract category/test_name from path
    rel="${binary#$BUILD_DIR/}"
    category="${rel%%/*}"
    rest="${rel#*/}"
    test_name="${rest%%/*}"
    test_key="${category}/${test_name}"

    # Skip SCREEN SECTION tests
    if [ "$category" = "$SKIP_CATEGORY" ]; then
        SKIPPED=$((SKIPPED + 1))
        printf "${YELLOW}  SKIP${RESET}  %-60s  (SCREEN SECTION)\n" "$test_key"
        continue
    fi

    TOTAL=$((TOTAL + 1))

    # Run with timeout, discard stdout/stderr, capture exit code
    EXIT_CODE=0
    timeout "$TIMEOUT_SEC" "$binary" </dev/null >/dev/null 2>&1 || EXIT_CODE=$?

    if [ "$EXIT_CODE" -eq 0 ]; then
        EXIT_0=$((EXIT_0 + 1))
        printf "${GREEN}  OK${RESET}    %-60s  (exit 0)\n" "$test_key"
    elif [ "$EXIT_CODE" -eq 124 ]; then
        # timeout returns 124 when the process times out
        TIMEDOUT=$((TIMEDOUT + 1))
        TIMEOUT_LIST="${TIMEOUT_LIST}  ${test_key}\n"
        printf "${YELLOW}  TOUT${RESET}  %-60s  (timeout %ds)\n" "$test_key" "$TIMEOUT_SEC"
    elif [ "$EXIT_CODE" -ge 128 ]; then
        # Signal: exit code = 128 + signal number (e.g., 139 = SIGSEGV)
        SIG=$((EXIT_CODE - 128))
        CRASHED=$((CRASHED + 1))
        CRASH_LIST="${CRASH_LIST}  ${test_key} (signal ${SIG})\n"
        printf "${RED}  CRASH${RESET} %-60s  (signal %d)\n" "$test_key" "$SIG"
    else
        EXIT_NONZERO=$((EXIT_NONZERO + 1))
        NONZERO_LIST="${NONZERO_LIST}  ${test_key} (exit ${EXIT_CODE})\n"
        printf "${YELLOW}  EXIT${RESET}  %-60s  (exit %d)\n" "$test_key" "$EXIT_CODE"
    fi
done < <(find "$BUILD_DIR" -name "*_hardened" -o -name "*_hardened.exe" | sort)

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

printf "\n${BOLD}=============================================${RESET}\n"
printf "${BOLD}  RUNTIME SMOKE TEST SUMMARY${RESET}\n"
printf "${BOLD}=============================================${RESET}\n\n"

printf "  Total tested:       %d\n" "$TOTAL"
printf "  ${GREEN}Clean exit (0):${RESET}    %d\n" "$EXIT_0"
printf "  ${YELLOW}Non-zero exit:${RESET}     %d\n" "$EXIT_NONZERO"
printf "  ${RED}Crash/signal:${RESET}      %d\n" "$CRASHED"
printf "  ${YELLOW}Timeout:${RESET}           %d\n" "$TIMEDOUT"
printf "  Skipped (screen):  %d\n" "$SKIPPED"
printf "  Elapsed:           %dm %ds\n" "$((ELAPSED / 60))" "$((ELAPSED % 60))"

# Write machine-readable log
cat > "$SMOKE_LOG" <<EOF
date=$(date -u +%Y-%m-%dT%H:%M:%SZ)
total=$TOTAL
exit_0=$EXIT_0
exit_nonzero=$EXIT_NONZERO
crashed=$CRASHED
timedout=$TIMEDOUT
skipped=$SKIPPED
elapsed_seconds=$ELAPSED
EOF

if [ -n "$NONZERO_LIST" ]; then
    printf "\n${YELLOW}Non-zero exits:${RESET}\n"
    printf "$NONZERO_LIST"
    {
        echo ""
        echo "non_zero_exits:"
        printf "$NONZERO_LIST"
    } >> "$SMOKE_LOG"
fi

if [ -n "$CRASH_LIST" ]; then
    printf "\n${RED}Crashes:${RESET}\n"
    printf "$CRASH_LIST"
    {
        echo ""
        echo "crashes:"
        printf "$CRASH_LIST"
    } >> "$SMOKE_LOG"
fi

if [ -n "$TIMEOUT_LIST" ]; then
    printf "\n${YELLOW}Timeouts:${RESET}\n"
    printf "$TIMEOUT_LIST"
    {
        echo ""
        echo "timeouts:"
        printf "$TIMEOUT_LIST"
    } >> "$SMOKE_LOG"
fi

printf "\n"

# Exit: 0 if no crashes, 1 if any crash detected
if [ "$CRASHED" -eq 0 ]; then
    printf "${GREEN}${BOLD}  RESULT: ZERO CRASHES${RESET}\n\n"
    exit 0
else
    printf "${RED}${BOLD}  RESULT: %d CRASHES DETECTED${RESET}\n\n" "$CRASHED"
    exit 1
fi
