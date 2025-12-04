#!/bin/bash
#
# Run Comprehensive E2E Streaming Tests
# Tests complete streaming workflows against live Restreamer server
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
TEST_BINARY="test_streaming_e2e"
BUILD_DIR="${BUILD_DIR:-../../build}"
TEST_EXEC="${BUILD_DIR}/tests/${TEST_BINARY}"

# Server configuration
RESTREAMER_URL="${RESTREAMER_URL:-rs2.rainmanjam.com}"
RESTREAMER_PORT="${RESTREAMER_PORT:-443}"
RESTREAMER_USER="${RESTREAMER_USER:-admin}"
RESTREAMER_PASS="${RESTREAMER_PASS:-tenn2jagWEE@##\$}"

print_header() {
    echo ""
    echo -e "${BLUE}========================================================================${NC}"
    echo -e "${BLUE} $1${NC}"
    echo -e "${BLUE}========================================================================${NC}"
    echo ""
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[i]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    print_header "Checking Prerequisites"

    # Check if test binary exists
    if [ ! -f "${TEST_EXEC}" ]; then
        print_error "Test binary not found: ${TEST_EXEC}"
        print_info "Please build the project first:"
        print_info "  cd ${BUILD_DIR}"
        print_info "  cmake .."
        print_info "  make ${TEST_BINARY}"
        return 1
    fi
    print_success "Test binary found: ${TEST_EXEC}"

    # Check if executable
    if [ ! -x "${TEST_EXEC}" ]; then
        print_warning "Test binary not executable, making it executable..."
        chmod +x "${TEST_EXEC}"
    fi
    print_success "Test binary is executable"

    # Check network connectivity to server
    print_info "Checking connectivity to ${RESTREAMER_URL}:${RESTREAMER_PORT}..."
    if command -v nc &> /dev/null; then
        if nc -z -w5 "${RESTREAMER_URL}" "${RESTREAMER_PORT}" 2>/dev/null; then
            print_success "Server is reachable"
        else
            print_warning "Could not verify server connectivity (may still work)"
        fi
    else
        print_warning "netcat (nc) not available, skipping connectivity check"
    fi

    return 0
}

# Run the E2E tests
run_tests() {
    print_header "Running E2E Streaming Tests"

    print_info "Test Configuration:"
    echo "  Server:   ${RESTREAMER_URL}:${RESTREAMER_PORT}"
    echo "  Username: ${RESTREAMER_USER}"
    echo "  Protocol: HTTPS"
    echo ""

    print_info "Starting test execution..."
    echo ""

    # Run the test binary
    if "${TEST_EXEC}"; then
        print_success "All tests passed!"
        return 0
    else
        EXIT_CODE=$?
        print_error "Tests failed with exit code: ${EXIT_CODE}"
        return ${EXIT_CODE}
    fi
}

# Cleanup function
cleanup() {
    print_header "Cleanup"
    print_info "Test processes should be automatically cleaned up by the test suite"
    print_info "If any processes remain, manually delete them from the Restreamer UI"
    echo ""
}

# Main execution
main() {
    print_header "OBS Polyemesis - Comprehensive E2E Streaming Tests"

    echo "Date: $(date)"
    echo "Host: $(hostname)"
    echo ""

    # Check prerequisites
    if ! check_prerequisites; then
        exit 1
    fi

    # Run tests
    if run_tests; then
        cleanup
        print_header "Test Execution Complete"
        print_success "All E2E streaming tests passed!"
        exit 0
    else
        EXIT_CODE=$?
        cleanup
        print_header "Test Execution Complete"
        print_error "Some tests failed. Please review the output above."
        exit ${EXIT_CODE}
    fi
}

# Handle script arguments
case "${1:-}" in
    -h|--help)
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  -h, --help     Show this help message"
        echo "  -v, --version  Show version information"
        echo ""
        echo "Environment Variables:"
        echo "  BUILD_DIR         Build directory (default: ../../build)"
        echo "  RESTREAMER_URL    Restreamer server URL (default: rs2.rainmanjam.com)"
        echo "  RESTREAMER_PORT   Restreamer server port (default: 443)"
        echo "  RESTREAMER_USER   Restreamer username (default: admin)"
        echo "  RESTREAMER_PASS   Restreamer password (default: tenn2jagWEE@##\$)"
        echo ""
        echo "Examples:"
        echo "  # Run with default settings"
        echo "  ./run-streaming-e2e-tests.sh"
        echo ""
        echo "  # Run with custom build directory"
        echo "  BUILD_DIR=/path/to/build ./run-streaming-e2e-tests.sh"
        echo ""
        echo "  # Run with custom server"
        echo "  RESTREAMER_URL=my-server.com RESTREAMER_USER=myuser RESTREAMER_PASS=mypass ./run-streaming-e2e-tests.sh"
        echo ""
        exit 0
        ;;
    -v|--version)
        echo "OBS Polyemesis E2E Streaming Tests v1.0"
        exit 0
        ;;
    "")
        # No arguments, run normally
        main
        ;;
    *)
        print_error "Unknown option: $1"
        echo "Use -h or --help for usage information"
        exit 1
        ;;
esac
