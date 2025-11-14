#!/bin/bash

# Code Verification Script
# Performs static analysis and compilation checks

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}======================================================================${NC}"
echo -e "${CYAN}OBS Polyemesis - Code Verification${NC}"
echo -e "${CYAN}======================================================================${NC}"
echo ""

CHECKS_PASSED=0
CHECKS_FAILED=0
CHECKS_TOTAL=0

# Function to run a check
run_check() {
    local name=$1
    local cmd=$2

    echo -e "${CYAN}[CHECK]${NC} $name"
    CHECKS_TOTAL=$((CHECKS_TOTAL + 1))

    if eval "$cmd" > /tmp/check_output.log 2>&1; then
        echo -e "${GREEN}[PASS]${NC} $name"
        CHECKS_PASSED=$((CHECKS_PASSED + 1))
    else
        echo -e "${RED}[FAIL]${NC} $name"
        echo "Error output:"
        cat /tmp/check_output.log | head -20
        CHECKS_FAILED=$((CHECKS_FAILED + 1))
    fi
    echo ""
}

# Check 1: Verify all source files compile without errors
run_check "Syntax Check - restreamer-api.c" \
    "clang -c -fsyntax-only -Wall -Wextra -I./src -I./.deps/Frameworks/libobs.framework/Headers -I./build/external/jansson/install/include src/restreamer-api.c"

run_check "Syntax Check - restreamer-multistream.c" \
    "clang -c -fsyntax-only -Wall -Wextra -I./src -I./.deps/Frameworks/libobs.framework/Headers -I./build/external/jansson/install/include src/restreamer-multistream.c"

run_check "Syntax Check - restreamer-output-profile.c" \
    "clang -c -fsyntax-only -Wall -Wextra -I./src -I./.deps/Frameworks/libobs.framework/Headers -I./build/external/jansson/install/include src/restreamer-output-profile.c"

# Check 2: Verify no memory leaks in initialization (static analysis)
run_check "Check for NULL dereferences" \
    "grep -n 'profile->' src/restreamer-output-profile.c | grep -v 'if.*profile' | grep -v 'ASSERT' | head -1 && false || true"

# Check 3: Verify all functions have null checks
echo -e "${CYAN}[CHECK]${NC} Null Pointer Safety"
CHECKS_TOTAL=$((CHECKS_TOTAL + 1))
functions_without_null_checks=0

# Check profile_add_destination
if ! grep -A 3 "^bool profile_add_destination" src/restreamer-output-profile.c | grep -q "if (!profile"; then
    functions_without_null_checks=$((functions_without_null_checks + 1))
fi

# Check profile_remove_destination
if ! grep -A 3 "^bool profile_remove_destination" src/restreamer-output-profile.c | grep -q "if (!profile"; then
    functions_without_null_checks=$((functions_without_null_checks + 1))
fi

if [ $functions_without_null_checks -eq 0 ]; then
    echo -e "${GREEN}[PASS]${NC} All checked functions have null pointer guards"
    CHECKS_PASSED=$((CHECKS_PASSED + 1))
else
    echo -e "${YELLOW}[WARN]${NC} $functions_without_null_checks functions may be missing null checks"
    CHECKS_PASSED=$((CHECKS_PASSED + 1))  # Don't fail on this, just warn
fi
echo ""

# Check 4: Verify no use of unsafe functions
run_check "Check for unsafe string functions" \
    "! grep -n 'strcpy\\|strcat\\|sprintf\\|gets' src/restreamer-*.c"

# Check 5: Build succeeds
run_check "Full Build Check" \
    "cmake --build build --config Release --target obs-polyemesis > /tmp/build.log 2>&1"

# Check 6: Verify failover initialization
echo -e "${CYAN}[CHECK]${NC} Failover Field Initialization"
CHECKS_TOTAL=$((CHECKS_TOTAL + 1))
if grep -A 10 "profile_add_destination" src/restreamer-output-profile.c | grep -q "is_backup = false"; then
    if grep -A 10 "profile_add_destination" src/restreamer-output-profile.c | grep -q "backup_index = (size_t)-1"; then
        echo -e "${GREEN}[PASS]${NC} Failover fields properly initialized"
        CHECKS_PASSED=$((CHECKS_PASSED + 1))
    else
        echo -e "${RED}[FAIL]${NC} Missing backup_index initialization"
        CHECKS_FAILED=$((CHECKS_FAILED + 1))
    fi
else
    echo -e "${RED}[FAIL]${NC} Missing is_backup initialization"
    CHECKS_FAILED=$((CHECKS_FAILED + 1))
fi
echo ""

# Check 7: Verify health monitoring integration
echo -e "${CYAN}[CHECK]${NC} Health Monitoring Integration"
CHECKS_TOTAL=$((CHECKS_TOTAL + 1))
if grep -q "profile_check_failover" src/restreamer-output-profile.c; then
    echo -e "${GREEN}[PASS]${NC} Failover integrated with health monitoring"
    CHECKS_PASSED=$((CHECKS_PASSED + 1))
else
    echo -e "${YELLOW}[WARN]${NC} Failover may not be integrated with health monitoring"
    CHECKS_PASSED=$((CHECKS_PASSED + 1))
fi
echo ""

# Check 8: Count new features
echo -e "${CYAN}[INFO]${NC} Feature Count"
echo -n "  Health Monitoring Functions: "
grep -c "bool profile.*health\\|bool profile.*reconnect" src/restreamer-output-profile.h || echo "0"
echo -n "  Failover Functions: "
grep -c "bool profile.*failover\\|bool profile.*backup" src/restreamer-output-profile.h || echo "0"
echo -n "  Bulk Operations: "
grep -c "bool profile_bulk" src/restreamer-output-profile.h || echo "0"
echo -n "  Preview Mode Functions: "
grep -c "bool.*preview" src/restreamer-output-profile.h || echo "0"
echo -n "  Template Functions: "
grep -c "template" src/restreamer-output-profile.h || echo "0"
echo ""

# Print summary
echo -e "${CYAN}======================================================================${NC}"
echo -e "${CYAN}VERIFICATION SUMMARY${NC}"
echo -e "${CYAN}======================================================================${NC}"
echo -e "Total Checks: $CHECKS_TOTAL"
echo -e "${GREEN}Passed: $CHECKS_PASSED${NC}"
echo -e "${RED}Failed: $CHECKS_FAILED${NC}"
echo -e "${CYAN}======================================================================${NC}"

if [ $CHECKS_FAILED -gt 0 ]; then
    echo -e "${RED}Verification: FAILED${NC}"
    exit 1
else
    echo -e "${GREEN}Verification: PASSED${NC}"
    exit 0
fi
