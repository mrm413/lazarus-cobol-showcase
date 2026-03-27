#!/usr/bin/env bash
# ============================================================================
# Lazarus C++17 Platform — Test Builder
# Torsova LLC — lazarus-systems.com
# Compiles and runs all tier tests using doctest framework.
# ============================================================================
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -Wall -Werror -Wextra -static -I${REPO}/include -I${REPO}"
TEST_DIR="${REPO}/tests"
BUILD_DIR="${REPO}/test_build"

mkdir -p "$BUILD_DIR"

TOTAL=0
PASS=0
FAIL=0
FAIL_LIST=""

if [ -t 1 ]; then
    GREEN='\033[0;32m'; RED='\033[0;31m'; CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'
else
    GREEN='' RED='' CYAN='' BOLD='' RESET=''
fi

# Build and run a single tier if specified, or all
TIER_FILTER="${1:-}"

for test_file in "$TEST_DIR"/test_tier*.cpp; do
    [ -f "$test_file" ] || continue
    base=$(basename "$test_file" .cpp)

    # Filter by tier if specified
    if [ -n "$TIER_FILTER" ] && [[ "$base" != *"$TIER_FILTER"* ]]; then
        continue
    fi

    TOTAL=$((TOTAL + 1))
    exe="$BUILD_DIR/${base}.exe"

    printf "${CYAN}Building${RESET} %-40s " "$base"

    if $CXX $CXXFLAGS "$test_file" -o "$exe" 2>"$BUILD_DIR/${base}_compile.log"; then
        # Run the test
        if "$exe" 2>"$BUILD_DIR/${base}_run.log"; then
            # Extract test count from doctest output
            count=$(grep -oP 'assertions: \K\d+' "$BUILD_DIR/${base}_run.log" 2>/dev/null || echo "?")
            printf "${GREEN}PASS${RESET} (%s assertions)\n" "$count"
            PASS=$((PASS + 1))
        else
            printf "${RED}FAIL (runtime)${RESET}\n"
            FAIL=$((FAIL + 1))
            FAIL_LIST="${FAIL_LIST}  ${base} (runtime)\n"
            tail -5 "$BUILD_DIR/${base}_run.log"
        fi
    else
        printf "${RED}FAIL (compile)${RESET}\n"
        FAIL=$((FAIL + 1))
        FAIL_LIST="${FAIL_LIST}  ${base} (compile)\n"
        tail -5 "$BUILD_DIR/${base}_compile.log"
    fi
done

printf "\n${BOLD}========================================${RESET}\n"
printf "${BOLD}  TEST SUMMARY${RESET}\n"
printf "${BOLD}========================================${RESET}\n"
printf "  Total suites: %d\n" "$TOTAL"
printf "  ${GREEN}PASS:${RESET} %d\n" "$PASS"
printf "  ${RED}FAIL:${RESET} %d\n" "$FAIL"

if [ -n "$FAIL_LIST" ]; then
    printf "\nFailed:\n"
    printf "$FAIL_LIST"
fi

printf "\n"
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
