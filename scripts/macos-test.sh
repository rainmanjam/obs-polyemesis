#!/bin/bash
# Run tests for OBS Polyemesis on macOS
# Usage: ./scripts/macos-test.sh [options]

set -e

# Configuration
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
TEST_FILTER="${TEST_FILTER:-}"
VERBOSE="${VERBOSE:-0}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

# Display usage
usage() {
    cat << EOF
Usage: $0 [options]

Run test suite for OBS Polyemesis on macOS.

Options:
    -h, --help              Show this help message
    --build-dir DIR         Build directory (default: build)
    --build-type TYPE       Build type: Release, Debug (default: Release)
    --filter PATTERN        Run only tests matching pattern
    --build-first           Build with tests enabled before running
    -v, --verbose           Verbose output

Environment Variables:
    BUILD_DIR               Override build directory
    BUILD_TYPE              Override build type
    TEST_FILTER             Filter tests by name pattern
    VERBOSE                 Enable verbose output (1 or 0)

Examples:
    # Run all tests
    $0

    # Build and run tests
    $0 --build-first

    # Run specific test
    $0 --filter "restreamer.*"

    # Run tests with verbose output
    $0 -v

EOF
    exit 0
}

# Parse arguments
BUILD_FIRST=0
while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            usage
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --filter)
            TEST_FILTER="$2"
            shift 2
            ;;
        --build-first)
            BUILD_FIRST=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Main execution
main() {
    log_info "macOS Test Runner for OBS Polyemesis"
    log_debug "Build directory: $BUILD_DIR"
    log_debug "Build type: $BUILD_TYPE"

    # Build with tests if requested
    if [ $BUILD_FIRST -eq 1 ]; then
        log_info "Building with tests enabled..."
        ENABLE_TESTING=ON ./scripts/macos-build.sh --build-type "$BUILD_TYPE" --build-dir "$BUILD_DIR"
    fi

    # Check if build directory exists
    if [ ! -d "$BUILD_DIR" ]; then
        log_error "Build directory not found: $BUILD_DIR"
        log_info "Run with --build-first to build the project first"
        exit 1
    fi

    # Find test executable
    TEST_PATHS=(
        "$BUILD_DIR/tests/$BUILD_TYPE/obs-polyemesis-tests"
        "$BUILD_DIR/tests/obs-polyemesis-tests"
        "$BUILD_DIR/$BUILD_TYPE/tests/obs-polyemesis-tests"
        "$BUILD_DIR/obs-polyemesis-tests"
    )

    TEST_EXE=""
    for path in "${TEST_PATHS[@]}"; do
        if [ -f "$path" ]; then
            TEST_EXE="$path"
            log_debug "Found test executable: $TEST_EXE"
            break
        fi
    done

    if [ -z "$TEST_EXE" ]; then
        log_error "Test executable not found"
        log_info "Tests may not be built. Try running:"
        log_info "  $0 --build-first"
        log_info ""
        log_info "Or build manually with:"
        log_info "  ENABLE_TESTING=ON ./scripts/macos-build.sh"
        exit 1
    fi

    log_info "✓ Found test executable: $TEST_EXE"

    # Run tests
    log_info "Running tests..."
    echo ""

    TEST_ARGS=()
    if [ -n "$TEST_FILTER" ]; then
        log_info "Running tests matching: $TEST_FILTER"
        TEST_ARGS+=(--gtest_filter="$TEST_FILTER")
    fi

    if [ "$VERBOSE" = "1" ]; then
        TEST_ARGS+=(--gtest_print_time=1)
    fi

    # Run the tests
    if "$TEST_EXE" "${TEST_ARGS[@]}"; then
        EXIT_CODE=0
        echo ""
        log_info "✓ All tests passed"
    else
        EXIT_CODE=$?
        echo ""
        log_error "✗ Tests failed with exit code $EXIT_CODE"
    fi

    exit $EXIT_CODE
}

main
