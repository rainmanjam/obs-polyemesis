#!/bin/bash
# OBS Polyemesis - Unified End-to-End Test Runner
# Runs E2E tests across all available platforms

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Test results
PLATFORM_COUNT=0
PLATFORMS_PASSED=0
PLATFORMS_FAILED=0
TOTAL_TESTS=0
TOTAL_PASSED=0
TOTAL_FAILED=0

# Configuration
TEST_MODE="${1:-quick}"
REMOTE_LINUX="${REMOTE_LINUX:-}"
REMOTE_WINDOWS="${REMOTE_WINDOWS:-windows-act}"

# Logging functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_section() {
    echo ""
    echo -e "${CYAN}========================================${NC}"
    echo -e "${CYAN} $1${NC}"
    echo -e "${CYAN}========================================${NC}"
    echo ""
}

# Print header
print_header() {
    echo "========================================"
    echo " OBS Polyemesis - Multi-Platform E2E Tests"
    echo "========================================"
    echo ""
    echo "Test Mode: $TEST_MODE"
    echo "Date: $(date)"
    echo ""
}

# Detect current platform
detect_platform() {
    case "$(uname -s)" in
        Darwin*)    echo "macos" ;;
        Linux*)     echo "linux" ;;
        CYGWIN*|MINGW*|MSYS*) echo "windows" ;;
        *)          echo "unknown" ;;
    esac
}

# Run macOS tests
run_macos_tests() {
    log_section "macOS End-to-End Tests"

    PLATFORM_COUNT=$((PLATFORM_COUNT + 1))

    if [ ! -f "$SCRIPT_DIR/macos/e2e-test-macos.sh" ]; then
        log_error "macOS test script not found"
        PLATFORMS_FAILED=$((PLATFORMS_FAILED + 1))
        return 1
    fi

    log_info "Running macOS E2E tests..."

    if "$SCRIPT_DIR/macos/e2e-test-macos.sh" "$TEST_MODE" > /tmp/macos-e2e-output.txt 2>&1; then
        # Parse results
        local results=$(tail -20 /tmp/macos-e2e-output.txt)
        local tests_run=$(echo "$results" | grep "Total Tests:" | awk '{print $3}')
        local tests_passed=$(echo "$results" | grep "Passed:" | awk '{print $2}')
        local tests_failed=$(echo "$results" | grep "Failed:" | awk '{print $2}')

        TOTAL_TESTS=$((TOTAL_TESTS + tests_run))
        TOTAL_PASSED=$((TOTAL_PASSED + tests_passed))
        TOTAL_FAILED=$((TOTAL_FAILED + tests_failed))

        log_success "macOS tests passed: $tests_passed/$tests_run"
        PLATFORMS_PASSED=$((PLATFORMS_PASSED + 1))

        # Show summary
        tail -30 /tmp/macos-e2e-output.txt | grep -A 10 "Test Summary"
    else
        log_error "macOS tests failed"
        PLATFORMS_FAILED=$((PLATFORMS_FAILED + 1))

        # Show error output
        echo ""
        tail -50 /tmp/macos-e2e-output.txt
    fi
}

# Run Linux tests
run_linux_tests() {
    log_section "Linux End-to-End Tests"

    PLATFORM_COUNT=$((PLATFORM_COUNT + 1))

    if [ ! -f "$SCRIPT_DIR/linux/e2e-test-linux.sh" ]; then
        log_error "Linux test script not found"
        PLATFORMS_FAILED=$((PLATFORMS_FAILED + 1))
        return 1
    fi

    # Check if running on Linux or need remote execution
    if [ "$(detect_platform)" = "linux" ]; then
        log_info "Running Linux E2E tests locally..."

        if "$SCRIPT_DIR/linux/e2e-test-linux.sh" "$TEST_MODE" > /tmp/linux-e2e-output.txt 2>&1; then
            # Parse results
            local results=$(tail -20 /tmp/linux-e2e-output.txt)
            local tests_run=$(echo "$results" | grep "Total Tests:" | awk '{print $3}')
            local tests_passed=$(echo "$results" | grep "Passed:" | awk '{print $2}')
            local tests_failed=$(echo "$results" | grep "Failed:" | awk '{print $2}')

            TOTAL_TESTS=$((TOTAL_TESTS + tests_run))
            TOTAL_PASSED=$((TOTAL_PASSED + tests_passed))
            TOTAL_FAILED=$((TOTAL_FAILED + tests_failed))

            log_success "Linux tests passed: $tests_passed/$tests_run"
            PLATFORMS_PASSED=$((PLATFORMS_PASSED + 1))

            # Show summary
            tail -30 /tmp/linux-e2e-output.txt | grep -A 10 "Test Summary"
        else
            log_error "Linux tests failed"
            PLATFORMS_FAILED=$((PLATFORMS_FAILED + 1))

            # Show error output
            echo ""
            tail -50 /tmp/linux-e2e-output.txt
        fi
    elif [ -n "$REMOTE_LINUX" ]; then
        log_info "Running Linux E2E tests remotely on $REMOTE_LINUX..."
        log_warning "Remote Linux testing not yet implemented"
        PLATFORMS_FAILED=$((PLATFORMS_FAILED + 1))
    else
        log_warning "Skipping Linux tests (not on Linux platform and no remote host configured)"
        PLATFORM_COUNT=$((PLATFORM_COUNT - 1))
    fi
}

# Run Windows tests
run_windows_tests() {
    log_section "Windows End-to-End Tests"

    PLATFORM_COUNT=$((PLATFORM_COUNT + 1))

    if [ ! -f "$SCRIPT_DIR/windows/e2e-test-windows.ps1" ]; then
        log_error "Windows test script not found"
        PLATFORMS_FAILED=$((PLATFORMS_FAILED + 1))
        return 1
    fi

    # Check if running on Windows or need remote execution
    if [ "$(detect_platform)" = "windows" ]; then
        log_info "Running Windows E2E tests locally..."

        if powershell -ExecutionPolicy Bypass -File "$SCRIPT_DIR/windows/e2e-test-windows.ps1" -TestMode "$TEST_MODE" > /tmp/windows-e2e-output.txt 2>&1; then
            # Parse results
            local results=$(tail -20 /tmp/windows-e2e-output.txt)
            local tests_run=$(echo "$results" | grep "Total Tests:" | awk '{print $3}')
            local tests_passed=$(echo "$results" | grep "Passed:" | awk '{print $2}')
            local tests_failed=$(echo "$results" | grep "Failed:" | awk '{print $2}')

            TOTAL_TESTS=$((TOTAL_TESTS + tests_run))
            TOTAL_PASSED=$((TOTAL_PASSED + tests_passed))
            TOTAL_FAILED=$((TOTAL_FAILED + tests_failed))

            log_success "Windows tests passed: $tests_passed/$tests_run"
            PLATFORMS_PASSED=$((PLATFORMS_PASSED + 1))

            # Show summary
            tail -30 /tmp/windows-e2e-output.txt | grep -A 10 "Test Summary"
        else
            log_error "Windows tests failed"
            PLATFORMS_FAILED=$((PLATFORMS_FAILED + 1))

            # Show error output
            echo ""
            tail -50 /tmp/windows-e2e-output.txt
        fi
    elif [ -n "$REMOTE_WINDOWS" ]; then
        log_info "Running Windows E2E tests remotely on $REMOTE_WINDOWS..."

        # Sync E2E test files to Windows
        local windows_test_dir="C:/act-workspace/obs-polyemesis/tests/e2e"

        log_info "Syncing E2E test files to Windows..."
        ssh "$REMOTE_WINDOWS" "mkdir -p $windows_test_dir/windows" 2>/dev/null || true

        scp "$SCRIPT_DIR/windows/e2e-test-windows.ps1" \
            "$REMOTE_WINDOWS:$windows_test_dir/windows/" >/dev/null 2>&1

        if ssh "$REMOTE_WINDOWS" "cd $windows_test_dir/windows && powershell -ExecutionPolicy Bypass -File .\\e2e-test-windows.ps1 -TestMode $TEST_MODE" > /tmp/windows-e2e-output.txt 2>&1; then
            # Parse results
            local results=$(tail -20 /tmp/windows-e2e-output.txt)
            local tests_run=$(echo "$results" | grep "Total Tests:" | awk '{print $3}')
            local tests_passed=$(echo "$results" | grep "Passed:" | awk '{print $2}')
            local tests_failed=$(echo "$results" | grep "Failed:" | awk '{print $2}')

            TOTAL_TESTS=$((TOTAL_TESTS + tests_run))
            TOTAL_PASSED=$((TOTAL_PASSED + tests_passed))
            TOTAL_FAILED=$((TOTAL_FAILED + tests_failed))

            log_success "Windows tests passed: $tests_passed/$tests_run"
            PLATFORMS_PASSED=$((PLATFORMS_PASSED + 1))

            # Show summary
            tail -30 /tmp/windows-e2e-output.txt | grep -A 10 "Test Summary"
        else
            log_error "Windows tests failed"
            PLATFORMS_FAILED=$((PLATFORMS_FAILED + 1))

            # Show error output
            echo ""
            tail -50 /tmp/windows-e2e-output.txt
        fi
    else
        log_warning "Skipping Windows tests (not on Windows platform and no remote host configured)"
        PLATFORM_COUNT=$((PLATFORM_COUNT - 1))
    fi
}

# Print final summary
print_summary() {
    log_section "Multi-Platform Test Summary"

    echo "Platforms Tested: $PLATFORM_COUNT"
    echo "Platforms Passed: $PLATFORMS_PASSED"
    echo "Platforms Failed: $PLATFORMS_FAILED"
    echo ""
    echo "Total Tests:      $TOTAL_TESTS"
    echo "Total Passed:     $TOTAL_PASSED"
    echo "Total Failed:     $TOTAL_FAILED"
    echo ""

    if [ $PLATFORMS_FAILED -eq 0 ] && [ $PLATFORM_COUNT -gt 0 ]; then
        log_success "✅ All platform tests passed!"
        return 0
    elif [ $PLATFORM_COUNT -eq 0 ]; then
        log_warning "⚠ No platforms were tested"
        return 1
    else
        log_error "❌ Some platform tests failed"
        return 1
    fi
}

# Main execution
main() {
    print_header

    # Detect current platform
    local current_platform=$(detect_platform)
    log_info "Current platform: $current_platform"
    echo ""

    # Run tests based on platform
    case "$current_platform" in
        macos)
            run_macos_tests
            run_linux_tests
            run_windows_tests
            ;;
        linux)
            run_linux_tests
            run_macos_tests
            run_windows_tests
            ;;
        windows)
            run_windows_tests
            run_macos_tests
            run_linux_tests
            ;;
        *)
            log_error "Unknown platform: $current_platform"
            exit 1
            ;;
    esac

    # Print summary
    print_summary
    exit $?
}

# Run main
main
