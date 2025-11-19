#!/usr/bin/env bash
# Run tests for OBS Polyemesis on all platforms
# Usage: ./scripts/test-all-platforms.sh [options]
# Requires: bash 4.0+ for associative arrays

set -e

# Check bash version
if [ "${BASH_VERSINFO[0]}" -lt 4 ]; then
    echo "Error: This script requires bash 4.0 or later"
    echo "You have: bash ${BASH_VERSION}"
    echo ""
    echo "On macOS, install via Homebrew:"
    echo "  brew install bash"
    exit 1
fi

# Configuration
TEST_MACOS=1
TEST_LINUX=1
TEST_WINDOWS=1
STOP_ON_ERROR=1
VERBOSE="${VERBOSE:-0}"
BUILD_FIRST=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Helper functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_debug() {
    if [ "$VERBOSE" = "1" ]; then
        echo -e "${BLUE}[DEBUG]${NC} $1"
    fi
}

log_platform() {
    echo -e "${CYAN}[PLATFORM]${NC} $1"
}

# Display usage
usage() {
    cat << EOF
Usage: $0 [options]

Run tests for OBS Polyemesis on all platforms sequentially.

This script runs tests on:
  1. macOS (local tests via scripts/macos-test.sh)
  2. Linux (local Docker tests via act)
  3. Windows (remote tests via scripts/windows-test.sh)

Options:
    -h, --help              Show this help message
    --skip-macos            Skip macOS tests
    --skip-linux            Skip Linux tests
    --skip-windows          Skip Windows tests
    --no-stop-on-error      Continue even if a platform test fails
    --build-first           Build with tests enabled before running
    -v, --verbose           Verbose output

Environment Variables:
    VERBOSE                 Enable verbose output (1 or 0)

Examples:
    # Run tests on all platforms
    $0

    # Build and test all platforms
    $0 --build-first

    # Test only macOS and Linux
    $0 --skip-windows

    # Continue on test failure
    $0 --no-stop-on-error

EOF
    exit 0
}

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            usage
            ;;
        --skip-macos)
            TEST_MACOS=0
            shift
            ;;
        --skip-linux)
            TEST_LINUX=0
            shift
            ;;
        --skip-windows)
            TEST_WINDOWS=0
            shift
            ;;
        --no-stop-on-error)
            STOP_ON_ERROR=0
            shift
            ;;
        --build-first)
            BUILD_FIRST=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            export VERBOSE
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Track results
declare -A TEST_RESULTS
declare -A TEST_TIMES

# Main execution
main() {
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  OBS Polyemesis - Multi-Platform Test Suite"
    echo "══════════════════════════════════════════════════════════════"
    echo ""

    log_info "Test configuration:"
    log_info "  Build first: $([ $BUILD_FIRST -eq 1 ] && echo 'Yes' || echo 'No')"
    log_info "  Stop on error: $([ $STOP_ON_ERROR -eq 1 ] && echo 'Yes' || echo 'No')"
    log_info ""

    PLATFORMS_TO_TEST=()
    [ $TEST_MACOS -eq 1 ] && PLATFORMS_TO_TEST+=("macOS")
    [ $TEST_LINUX -eq 1 ] && PLATFORMS_TO_TEST+=("Linux")
    [ $TEST_WINDOWS -eq 1 ] && PLATFORMS_TO_TEST+=("Windows")

    if [ ${#PLATFORMS_TO_TEST[@]} -eq 0 ]; then
        log_error "No platforms selected for testing"
        exit 1
    fi

    log_info "Platforms to test: ${PLATFORMS_TO_TEST[*]}"
    echo ""

    TOTAL_START=$(date +%s)

    # ==========================================
    # macOS Tests
    # ==========================================
    if [ $TEST_MACOS -eq 1 ]; then
        echo ""
        echo "────────────────────────────────────────────────────────────"
        log_platform "Running macOS tests (1/${#PLATFORMS_TO_TEST[@]})"
        echo "────────────────────────────────────────────────────────────"
        echo ""

        PLATFORM_START=$(date +%s)

        TEST_ARGS=()
        [ $BUILD_FIRST -eq 1 ] && TEST_ARGS+=(--build-first)
        [ "$VERBOSE" = "1" ] && TEST_ARGS+=(-v)

        if ./scripts/macos-test.sh "${TEST_ARGS[@]}"; then
            PLATFORM_END=$(date +%s)
            TEST_DURATION=$((PLATFORM_END - PLATFORM_START))
            TEST_RESULTS[macOS]="✓ Passed"
            TEST_TIMES[macOS]=$TEST_DURATION

            log_info "✓ macOS tests passed in ${TEST_DURATION}s"
        else
            PLATFORM_END=$(date +%s)
            TEST_DURATION=$((PLATFORM_END - PLATFORM_START))
            TEST_RESULTS[macOS]="✗ Failed"
            TEST_TIMES[macOS]=$TEST_DURATION

            log_error "✗ macOS tests failed after ${TEST_DURATION}s"

            if [ $STOP_ON_ERROR -eq 1 ]; then
                echo ""
                log_error "Stopping tests due to macOS failure"
                display_summary
                exit 1
            fi
        fi
    fi

    # ==========================================
    # Linux Tests
    # ==========================================
    if [ $TEST_LINUX -eq 1 ]; then
        echo ""
        echo "────────────────────────────────────────────────────────────"
        PLATFORM_NUM=1
        [ $TEST_MACOS -eq 1 ] && PLATFORM_NUM=2
        log_platform "Running Linux tests ($PLATFORM_NUM/${#PLATFORMS_TO_TEST[@]})"
        echo "────────────────────────────────────────────────────────────"
        echo ""

        PLATFORM_START=$(date +%s)

        # Use act to run Linux tests in Docker
        log_info "Running Linux tests via act (Docker)..."

        ACT_ARGS=(
            -W .github/workflows/automated-tests.yml
            -j build-linux
            --container-architecture linux/amd64
        )

        if [ "$VERBOSE" = "1" ]; then
            ACT_ARGS+=(--verbose)
        fi

        if act "${ACT_ARGS[@]}"; then
            PLATFORM_END=$(date +%s)
            TEST_DURATION=$((PLATFORM_END - PLATFORM_START))
            TEST_RESULTS[Linux]="✓ Passed"
            TEST_TIMES[Linux]=$TEST_DURATION

            log_info "✓ Linux tests passed in ${TEST_DURATION}s"
        else
            PLATFORM_END=$(date +%s)
            TEST_DURATION=$((PLATFORM_END - PLATFORM_START))
            TEST_RESULTS[Linux]="✗ Failed"
            TEST_TIMES[Linux]=$TEST_DURATION

            log_error "✗ Linux tests failed after ${TEST_DURATION}s"

            if [ $STOP_ON_ERROR -eq 1 ]; then
                echo ""
                log_error "Stopping tests due to Linux failure"
                display_summary
                exit 1
            fi
        fi
    fi

    # ==========================================
    # Windows Tests
    # ==========================================
    if [ $TEST_WINDOWS -eq 1 ]; then
        echo ""
        echo "────────────────────────────────────────────────────────────"
        PLATFORM_NUM=${#PLATFORMS_TO_TEST[@]}
        log_platform "Running Windows tests ($PLATFORM_NUM/${#PLATFORMS_TO_TEST[@]})"
        echo "────────────────────────────────────────────────────────────"
        echo ""

        PLATFORM_START=$(date +%s)

        # Check if Windows machine is accessible
        if ! ssh -q -o BatchMode=yes -o ConnectTimeout=5 "${WINDOWS_ACT_HOST:-windows-act}" exit 2>/dev/null; then
            log_error "Cannot connect to Windows test machine"
            log_info "Ensure Windows machine is accessible and SSH is configured"

            PLATFORM_END=$(date +%s)
            TEST_DURATION=$((PLATFORM_END - PLATFORM_START))
            TEST_RESULTS[Windows]="✗ Failed (No Connection)"
            TEST_TIMES[Windows]=$TEST_DURATION

            if [ $STOP_ON_ERROR -eq 1 ]; then
                echo ""
                log_error "Stopping tests due to Windows connection failure"
                display_summary
                exit 1
            fi
        else
            log_info "Running Windows tests via SSH..."

            if ./scripts/windows-test.sh; then
                PLATFORM_END=$(date +%s)
                TEST_DURATION=$((PLATFORM_END - PLATFORM_START))
                TEST_RESULTS[Windows]="✓ Passed"
                TEST_TIMES[Windows]=$TEST_DURATION

                log_info "✓ Windows tests passed in ${TEST_DURATION}s"
            else
                PLATFORM_END=$(date +%s)
                TEST_DURATION=$((PLATFORM_END - PLATFORM_START))
                TEST_RESULTS[Windows]="✗ Failed"
                TEST_TIMES[Windows]=$TEST_DURATION

                log_error "✗ Windows tests failed after ${TEST_DURATION}s"

                if [ $STOP_ON_ERROR -eq 1 ]; then
                    echo ""
                    log_error "Stopping tests due to Windows failure"
                    display_summary
                    exit 1
                fi
            fi
        fi
    fi

    TOTAL_END=$(date +%s)
    TOTAL_DURATION=$((TOTAL_END - TOTAL_START))

    # Display summary
    display_summary
}

# Display test summary
display_summary() {
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  Test Summary"
    echo "══════════════════════════════════════════════════════════════"
    echo ""

    # Display results
    printf "%-15s %-20s %s\n" "Platform" "Status" "Time"
    echo "────────────────────────────────────────────────────────────"

    FAILED_COUNT=0
    SUCCESS_COUNT=0

    for platform in "${!TEST_RESULTS[@]}"; do
        result="${TEST_RESULTS[$platform]}"
        time="${TEST_TIMES[$platform]}"

        printf "%-15s %-20s %ds\n" "$platform" "$result" "$time"

        if [[ $result == *"✗"* ]]; then
            FAILED_COUNT=$((FAILED_COUNT + 1))
        else
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        fi
    done

    echo "────────────────────────────────────────────────────────────"
    echo ""

    if [ -n "$TOTAL_DURATION" ]; then
        log_info "Total test time: ${TOTAL_DURATION}s"
    fi

    echo ""

    if [ $FAILED_COUNT -eq 0 ]; then
        log_info "✓ All platform tests passed!"
        echo ""
        log_info "Next steps:"
        log_info "  - Create packages: ./scripts/build-all-platforms.sh"
        log_info "  - Or use: make package"
        echo ""
        exit 0
    else
        log_error "✗ $FAILED_COUNT platform test(s) failed, $SUCCESS_COUNT passed"
        echo ""
        log_info "To retry individual platforms:"
        log_info "  macOS:   ./scripts/macos-test.sh"
        log_info "  Linux:   act -W .github/workflows/test.yaml"
        log_info "  Windows: ./scripts/windows-test.sh"
        echo ""
        exit 1
    fi
}

main
