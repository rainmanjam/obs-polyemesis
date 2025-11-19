#!/bin/bash
set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

PASSED=0
FAILED=0
WARNINGS=0

run_test() {
    local name="$1"
    local cmd="$2"

    log_info "Running: $name"
    if eval "$cmd"; then
        log_success "✓ $name"
        ((PASSED++))
    else
        log_error "✗ $name"
        ((FAILED++))
    fi
    echo ""
}

log_info "OBS Polyemesis - Comprehensive Act Testing"
echo "=========================================="
echo ""

# 1. Unit Tests (Ubuntu)
run_test "Unit Tests (Ubuntu)" \
    "act -j unit-tests-ubuntu -W .github/workflows/run-tests.yaml --pull=false"

# 2. Coverage (Linux)
run_test "Coverage (Linux)" \
    "act -j coverage-linux -W .github/workflows/coverage.yml --pull=false"

# 3. Lint (ShellCheck)
run_test "Lint (ShellCheck)" \
    "act -j shellcheck -W .github/workflows/lint.yaml --pull=false"

# 5. Docker Unit Tests (local)
run_test "Docker Unit Tests (Local)" \
    "./scripts/run-unit-tests-docker.sh"

# 6. Docker Coverage (local)
run_test "Docker Coverage (Local)" \
    "./scripts/run-coverage-docker.sh"

# Summary
echo "=========================================="
echo "Test Results Summary:"
echo "  ✓ Passed:   $PASSED"
echo "  ✗ Failed:   $FAILED"
echo "  ⚠ Warnings: $WARNINGS"
echo "=========================================="

if [ $FAILED -eq 0 ]; then
    log_success "All tests passed! Ready to push to GitHub."
    exit 0
else
    log_error "$FAILED test(s) failed. Please fix before pushing."
    exit 1
fi
