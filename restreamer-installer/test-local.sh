#!/bin/bash
#
# Local Testing Script for Restreamer Installer
# Tests syntax, logic, and common issues without requiring Linux
#

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASS=0
FAIL=0
WARN=0

print_test() {
    echo -e "${BLUE}TEST:${NC} $1"
}

pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((PASS++)) || true
}

fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    ((FAIL++)) || true
}

warn() {
    echo -e "${YELLOW}! WARN${NC}: $1"
    ((WARN++)) || true
}

echo "================================================"
echo "  Restreamer Installer - Local Tests"
echo "================================================"
echo ""

# Test 1: Bash syntax
print_test "Checking bash syntax"
if bash -n install.sh 2>/dev/null; then
    pass "install.sh syntax is valid"
else
    fail "install.sh has syntax errors"
fi

if bash -n uninstall.sh 2>/dev/null; then
    pass "uninstall.sh syntax is valid"
else
    fail "uninstall.sh has syntax errors"
fi

if bash -n diagnose.sh 2>/dev/null; then
    pass "diagnose.sh syntax is valid"
else
    fail "diagnose.sh has syntax errors"
fi

# Test 2: Shellcheck (if available)
print_test "Running ShellCheck"
if command -v shellcheck &> /dev/null; then
    if shellcheck -x install.sh 2>&1 | grep -q "SC"; then
        warn "install.sh has shellcheck warnings"
        shellcheck install.sh 2>&1 | head -20
    else
        pass "install.sh passes shellcheck"
    fi

    if shellcheck -x uninstall.sh 2>&1 | grep -q "SC"; then
        warn "uninstall.sh has shellcheck warnings"
    else
        pass "uninstall.sh passes shellcheck"
    fi

    if shellcheck -x diagnose.sh 2>&1 | grep -q "SC"; then
        warn "diagnose.sh has shellcheck warnings"
    else
        pass "diagnose.sh passes shellcheck"
    fi
else
    warn "shellcheck not installed - skipping (install with: brew install shellcheck)"
fi

# Test 3: Check for required functions
print_test "Checking required functions exist"
REQUIRED_FUNCS=(
    "check_root"
    "detect_distro"
    "install_docker"
    "get_user_input"
    "verify_dns"
    "cleanup_installation"
)

for func in "${REQUIRED_FUNCS[@]}"; do
    if grep -q "^${func}()" install.sh; then
        pass "Function $func exists"
    else
        fail "Function $func is missing"
    fi
done

# Test 4: Check state tracking variables
print_test "Checking state tracking variables"
STATE_VARS=(
    "INSTALLATION_STARTED"
    "DOCKER_INSTALLED_BY_US"
    "RESTREAMER_CONTAINER_CREATED"
    "WATCHTOWER_INSTALLED_BY_US"
    "SCRIPTS_CREATED"
    "FIREWALL_CONFIGURED"
)

for var in "${STATE_VARS[@]}"; do
    if grep -q "$var=false" install.sh; then
        pass "State variable $var initialized"
    else
        fail "State variable $var not initialized"
    fi
done

# Test 5: Check trap handlers
print_test "Checking trap handlers"
if grep -q "trap cleanup_installation EXIT" install.sh; then
    pass "EXIT trap handler configured"
else
    fail "EXIT trap handler missing"
fi

if grep -q "trap.*INT" install.sh; then
    pass "INT (Ctrl+C) trap handler configured"
else
    fail "INT trap handler missing"
fi

# Test 6: Check for placeholder URLs
print_test "Checking for placeholder URLs"
if grep -q "\[YOUR-USERNAME\]" install.sh uninstall.sh diagnose.sh README.md QUICKSTART.md 2>/dev/null; then
    fail "Found placeholder URLs that need to be updated"
    grep -n "\[YOUR-USERNAME\]" install.sh uninstall.sh diagnose.sh README.md QUICKSTART.md 2>/dev/null | head -5
else
    pass "No placeholder URLs found"
fi

if grep -q "\[user\]" install.sh uninstall.sh diagnose.sh 2>/dev/null; then
    fail "Found [user] placeholders"
else
    pass "No [user] placeholders found"
fi

# Test 7: Check README has TOC
print_test "Checking README structure"
if grep -q "## Table of Contents" README.md; then
    pass "README has Table of Contents"
else
    fail "README missing Table of Contents"
fi

# Test 8: Check for common issues
print_test "Checking for common scripting issues"

# Check for unquoted variables
if grep -E '\$[A-Z_]+[^"]' install.sh | grep -v "echo\|print_\|#" | head -1 > /dev/null; then
    warn "Found potentially unquoted variables (may be false positive)"
else
    pass "No obvious unquoted variable issues"
fi

# Check for hardcoded paths
if grep -q "/tmp/\|/var/\|/etc/\|/usr/" install.sh; then
    pass "Uses standard system paths"
else
    warn "Missing standard system paths"
fi

# Test 9: Check error handling
print_test "Checking error handling"
if grep -q "set -e" install.sh; then
    pass "Script uses 'set -e' for error handling"
else
    warn "Script doesn't use 'set -e'"
fi

if grep -q "error_exit" install.sh; then
    pass "Has error_exit function"
else
    fail "Missing error_exit function"
fi

# Test 10: Check documentation completeness
print_test "Checking documentation"
REQUIRED_SECTIONS=(
    "Quick Start"
    "Installation Flow"
    "Smart Rollback System"
    "Troubleshooting"
    "Requirements"
)

for section in "${REQUIRED_SECTIONS[@]}"; do
    if grep -q "## $section" README.md; then
        pass "README has '$section' section"
    else
        fail "README missing '$section' section"
    fi
done

# Test 11: Check for DNS verification
print_test "Checking DNS verification logic"
if grep -q "verify_dns" install.sh; then
    pass "DNS verification function exists"
else
    fail "DNS verification missing"
fi

if grep -q "dig\|host\|nslookup" install.sh; then
    pass "Uses DNS lookup tools"
else
    fail "Missing DNS lookup commands"
fi

# Test 12: Check rollback functionality
print_test "Checking rollback functionality"
if grep -q "cleanup_installation" install.sh; then
    pass "Cleanup function exists"
else
    fail "Cleanup function missing"
fi

if grep -q "Would you like to remove" install.sh; then
    pass "Asks user before cleanup"
else
    fail "Doesn't ask user before cleanup"
fi

# Test 13: Check permissions on scripts
print_test "Checking file permissions"
for script in install.sh uninstall.sh diagnose.sh; do
    if [ -x "$script" ]; then
        pass "$script is executable"
    else
        warn "$script is not executable (chmod +x $script)"
    fi
done

# Test 14: Check for security issues
print_test "Checking for security issues"
if grep -q "curl.*sudo bash" README.md; then
    pass "Uses sudo with curl pipe (expected)"
else
    warn "Curl pipe pattern not found in README"
fi

# Check for eval usage (potential security risk)
if grep "eval.*\$" install.sh | grep -v "PKG_" | grep -v "#" > /dev/null; then
    warn "Found eval with user input (check for safety)"
else
    pass "No unsafe eval usage detected"
fi

# Summary
echo ""
echo "================================================"
echo "  Test Summary"
echo "================================================"
echo -e "${GREEN}Passed:${NC}  $PASS"
echo -e "${YELLOW}Warnings:${NC} $WARN"
echo -e "${RED}Failed:${NC}  $FAIL"
echo ""

if [ $FAIL -gt 0 ]; then
    echo -e "${RED}⚠ Tests FAILED - please fix issues before deployment${NC}"
    exit 1
elif [ $WARN -gt 0 ]; then
    echo -e "${YELLOW}⚠ Tests PASSED with warnings - review before deployment${NC}"
    exit 0
else
    echo -e "${GREEN}✓ All tests PASSED - ready for deployment${NC}"
    exit 0
fi
