#!/usr/bin/env bash
# OBS Polyemesis - Comprehensive Test Runner
# Executes all test categories with detailed reporting

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_RESULTS_DIR="${PROJECT_ROOT}/test-results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Test categories to run
RUN_UNIT_TESTS=1
RUN_BUILD_TESTS=1
RUN_INTEGRATION_TESTS=1
RUN_E2E_TESTS=0  # Disabled by default (requires OBS)
RUN_RESTREAMER_TESTS=0  # Disabled by default (requires server)
RUN_PLATFORM_TESTS=1

# Platforms to test
TEST_MACOS=1
TEST_LINUX=1
TEST_WINDOWS=0  # Disabled by default (requires Windows machine)

# Test results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    PASSED_TESTS=$((PASSED_TESTS + 1))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    FAILED_TESTS=$((FAILED_TESTS + 1))
}

log_skip() {
    echo -e "${YELLOW}[SKIP]${NC} $1"
    SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
}

log_section() {
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo -e "${CYAN}$1${NC}"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
}

run_test_command() {
    local test_name="$1"
    local command="$2"
    local log_file="$3"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    log_info "Running: $test_name"

    if eval "$command" > "$log_file" 2>&1; then
        log_success "$test_name"
        return 0
    else
        log_fail "$test_name"
        log_info "  Log: $log_file"
        return 1
    fi
}

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        --skip-unit)
            RUN_UNIT_TESTS=0
            shift
            ;;
        --skip-build)
            RUN_BUILD_TESTS=0
            shift
            ;;
        --skip-integration)
            RUN_INTEGRATION_TESTS=0
            shift
            ;;
        --enable-e2e)
            RUN_E2E_TESTS=1
            shift
            ;;
        --enable-restreamer)
            RUN_RESTREAMER_TESTS=1
            shift
            ;;
        --enable-windows)
            TEST_WINDOWS=1
            shift
            ;;
        --skip-macos)
            TEST_MACOS=0
            shift
            ;;
        --skip-linux)
            TEST_LINUX=0
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --skip-unit           Skip unit tests"
            echo "  --skip-build          Skip build tests"
            echo "  --skip-integration    Skip integration tests"
            echo "  --enable-e2e          Enable E2E tests (requires OBS)"
            echo "  --enable-restreamer   Enable Restreamer integration tests"
            echo "  --enable-windows      Enable Windows tests"
            echo "  --skip-macos          Skip macOS tests"
            echo "  --skip-linux          Skip Linux tests"
            echo "  -h, --help            Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Initialize test results directory
mkdir -p "$TEST_RESULTS_DIR"
TEST_RUN_DIR="$TEST_RESULTS_DIR/run_${TIMESTAMP}"
mkdir -p "$TEST_RUN_DIR"

log_section "OBS Polyemesis - Comprehensive Test Suite"
log_info "Test run ID: $TIMESTAMP"
log_info "Results dir: $TEST_RUN_DIR"
echo ""

START_TIME=$(date +%s)

# ==========================================
# Category 1: Unit Tests
# ==========================================
if [ $RUN_UNIT_TESTS -eq 1 ]; then
    log_section "Category 1: Unit Tests"

    cd "$PROJECT_ROOT"

    # Check if build directory exists
    if [ -d "build" ]; then
        run_test_command \
            "CMake Unit Tests" \
            "cd build && ctest --output-on-failure" \
            "$TEST_RUN_DIR/unit-tests.log"
    else
        log_skip "Unit tests (build directory not found)"
    fi

    # Run Qt-based unit tests if available
    if [ -f "build_qt_tests/tests/Debug/obs-polyemesis-tests" ]; then
        run_test_command \
            "Qt Unit Tests" \
            "./build_qt_tests/tests/Debug/obs-polyemesis-tests" \
            "$TEST_RUN_DIR/qt-unit-tests.log"
    fi
fi

# ==========================================
# Category 2: Build Tests
# ==========================================
if [ $RUN_BUILD_TESTS -eq 1 ]; then
    log_section "Category 2: Build Tests"

    # macOS build test
    if [ $TEST_MACOS -eq 1 ] && [ "$(uname)" = "Darwin" ]; then
        run_test_command \
            "macOS Build (Debug)" \
            "cmake -S . -B build_test_debug -G Xcode -DCMAKE_BUILD_TYPE=Debug && cmake --build build_test_debug --config Debug" \
            "$TEST_RUN_DIR/build-macos-debug.log"

        run_test_command \
            "macOS Build (Release)" \
            "cmake -S . -B build_test_release -G Xcode -DCMAKE_BUILD_TYPE=Release && cmake --build build_test_release --config Release" \
            "$TEST_RUN_DIR/build-macos-release.log"
    fi

    # Linux build test via Docker/act
    if [ $TEST_LINUX -eq 1 ]; then
        if command -v act >/dev/null 2>&1; then
            run_test_command \
                "Linux Build (via act)" \
                "act -W .github/workflows/automated-tests.yml -j build-linux --platform ubuntu-latest=ubuntu:22.04" \
                "$TEST_RUN_DIR/build-linux-act.log"
        else
            log_skip "Linux build via act (act not installed)"
        fi
    fi
fi

# ==========================================
# Category 3: Integration Tests
# ==========================================
if [ $RUN_INTEGRATION_TESTS -eq 1 ]; then
    log_section "Category 3: Integration Tests"

    # API integration tests
    if [ -f "tests/integration/test_api.sh" ]; then
        run_test_command \
            "API Integration Tests" \
            "./tests/integration/test_api.sh" \
            "$TEST_RUN_DIR/integration-api.log"
    fi

    # Process management tests
    if [ -f "tests/integration/test_process.sh" ]; then
        run_test_command \
            "Process Management Tests" \
            "./tests/integration/test_process.sh" \
            "$TEST_RUN_DIR/integration-process.log"
    fi
fi

# ==========================================
# Category 4: End-to-End Tests
# ==========================================
if [ $RUN_E2E_TESTS -eq 1 ]; then
    log_section "Category 4: End-to-End Tests"

    # macOS E2E
    if [ $TEST_MACOS -eq 1 ] && [ -f "tests/e2e/macos/e2e-test-macos.sh" ]; then
        run_test_command \
            "macOS E2E Tests" \
            "./tests/e2e/macos/e2e-test-macos.sh quick" \
            "$TEST_RUN_DIR/e2e-macos.log"
    fi

    # Linux E2E
    if [ $TEST_LINUX -eq 1 ] && [ -f "tests/e2e/linux/e2e-test-linux.sh" ]; then
        run_test_command \
            "Linux E2E Tests" \
            "./tests/e2e/linux/e2e-test-linux.sh" \
            "$TEST_RUN_DIR/e2e-linux.log"
    fi
else
    log_skip "E2E tests (use --enable-e2e to run)"
fi

# ==========================================
# Category 5: Restreamer Integration
# ==========================================
if [ $RUN_RESTREAMER_TESTS -eq 1 ]; then
    log_section "Category 5: Restreamer Integration Tests"

    # Connection test
    if [ -f "test-connection-settings.sh" ]; then
        run_test_command \
            "Restreamer Connection Test" \
            "./test-connection-settings.sh" \
            "$TEST_RUN_DIR/restreamer-connection.log"
    fi

    # Streaming tests (if implemented)
    if [ -f "tests/scenarios/test-vertical-streaming.sh" ]; then
        run_test_command \
            "Vertical Streaming Test" \
            "./tests/scenarios/test-vertical-streaming.sh" \
            "$TEST_RUN_DIR/streaming-vertical.log"
    fi

    if [ -f "tests/scenarios/test-horizontal-streaming.sh" ]; then
        run_test_command \
            "Horizontal Streaming Test" \
            "./tests/scenarios/test-horizontal-streaming.sh" \
            "$TEST_RUN_DIR/streaming-horizontal.log"
    fi
else
    log_skip "Restreamer integration tests (use --enable-restreamer to run)"
fi

# ==========================================
# Category 6: Platform Tests
# ==========================================
if [ $RUN_PLATFORM_TESTS -eq 1 ]; then
    log_section "Category 6: Cross-Platform Tests"

    if [ -f "$SCRIPT_DIR/test-all-platforms.sh" ]; then
        PLATFORM_ARGS=()
        [ $TEST_MACOS -eq 0 ] && PLATFORM_ARGS+=(--skip-macos)
        [ $TEST_LINUX -eq 0 ] && PLATFORM_ARGS+=(--skip-linux)
        [ $TEST_WINDOWS -eq 0 ] && PLATFORM_ARGS+=(--skip-windows)

        run_test_command \
            "All Platform Tests" \
            "$SCRIPT_DIR/test-all-platforms.sh ${PLATFORM_ARGS[*]}" \
            "$TEST_RUN_DIR/platform-all.log"
    fi
fi

# ==========================================
# Test Summary
# ==========================================
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

log_section "Test Summary"

echo "Test Results:"
echo "  Total tests:    $TOTAL_TESTS"
echo -e "  Passed:         ${GREEN}$PASSED_TESTS${NC}"
echo -e "  Failed:         ${RED}$FAILED_TESTS${NC}"
echo -e "  Skipped:        ${YELLOW}$SKIPPED_TESTS${NC}"
echo ""
echo "Duration: ${DURATION}s"
echo "Results saved to: $TEST_RUN_DIR"
echo ""

# Generate summary report
cat > "$TEST_RUN_DIR/summary.txt" << EOF
OBS Polyemesis - Test Summary
=============================

Date: $(date)
Duration: ${DURATION}s

Results:
--------
Total:   $TOTAL_TESTS
Passed:  $PASSED_TESTS
Failed:  $FAILED_TESTS
Skipped: $SKIPPED_TESTS

Pass Rate: $(( TOTAL_TESTS > 0 ? PASSED_TESTS * 100 / TOTAL_TESTS : 0 ))%

Test Categories:
----------------
Unit Tests:        $([ $RUN_UNIT_TESTS -eq 1 ] && echo "Run" || echo "Skipped")
Build Tests:       $([ $RUN_BUILD_TESTS -eq 1 ] && echo "Run" || echo "Skipped")
Integration Tests: $([ $RUN_INTEGRATION_TESTS -eq 1 ] && echo "Run" || echo "Skipped")
E2E Tests:         $([ $RUN_E2E_TESTS -eq 1 ] && echo "Run" || echo "Skipped")
Restreamer Tests:  $([ $RUN_RESTREAMER_TESTS -eq 1 ] && echo "Run" || echo "Skipped")
Platform Tests:    $([ $RUN_PLATFORM_TESTS -eq 1 ] && echo "Run" || echo "Skipped")

Platforms:
----------
macOS:   $([ $TEST_MACOS -eq 1 ] && echo "Tested" || echo "Skipped")
Linux:   $([ $TEST_LINUX -eq 1 ] && echo "Tested" || echo "Skipped")
Windows: $([ $TEST_WINDOWS -eq 1 ] && echo "Tested" || echo "Skipped")

Log Files:
----------
$(ls -1 "$TEST_RUN_DIR"/*.log 2>/dev/null || echo "No log files")

EOF

cat "$TEST_RUN_DIR/summary.txt"

# Exit with appropriate code
if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}✅ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ $FAILED_TESTS test(s) failed${NC}"
    echo ""
    echo "To view failed test logs:"
    echo "  ls -lh $TEST_RUN_DIR/*.log"
    exit 1
fi
