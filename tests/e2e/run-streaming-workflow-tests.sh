#!/bin/bash
#
# Run End-to-End Streaming Workflow Tests
#
# This script builds and runs the E2E streaming workflow tests against
# a live Restreamer server.
#
# Usage:
#   ./run-streaming-workflow-tests.sh [options]
#
# Options:
#   --build-only    Only build, don't run tests
#   --verbose       Run with verbose output
#   --clean         Clean build before running
#   --help          Show this help message

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/../.."
BUILD_DIR="$PROJECT_ROOT/build"

# Options
BUILD_ONLY=false
VERBOSE=false
CLEAN_BUILD=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --build-only    Only build, don't run tests"
            echo "  --verbose       Run with verbose output"
            echo "  --clean         Clean build before running"
            echo "  --help          Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Error: Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Print header
echo -e "${BLUE}========================================================================${NC}"
echo -e "${BLUE}  OBS Polyemesis - E2E Streaming Workflow Tests${NC}"
echo -e "${BLUE}========================================================================${NC}"
echo ""

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}[CLEAN] Removing build directory...${NC}"
    rm -rf "$BUILD_DIR"
    echo -e "${GREEN}[CLEAN] Done${NC}"
    echo ""
fi

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}[BUILD] Creating build directory...${NC}"
    mkdir -p "$BUILD_DIR"
fi

# Configure CMake
echo -e "${YELLOW}[CMAKE] Configuring project...${NC}"
cd "$BUILD_DIR"
cmake .. || {
    echo -e "${RED}[ERROR] CMake configuration failed${NC}"
    exit 1
}
echo -e "${GREEN}[CMAKE] Configuration complete${NC}"
echo ""

# Build test executable
echo -e "${YELLOW}[BUILD] Building test_e2e_streaming_workflow...${NC}"
make test_e2e_streaming_workflow || {
    echo -e "${RED}[ERROR] Build failed${NC}"
    exit 1
}
echo -e "${GREEN}[BUILD] Build complete${NC}"
echo ""

# Check if test binary exists
TEST_BINARY="$BUILD_DIR/tests/test_e2e_streaming_workflow"
if [ ! -f "$TEST_BINARY" ]; then
    echo -e "${RED}[ERROR] Test binary not found: $TEST_BINARY${NC}"
    exit 1
fi

# Exit if build-only mode
if [ "$BUILD_ONLY" = true ]; then
    echo -e "${GREEN}[SUCCESS] Build completed successfully${NC}"
    echo -e "${BLUE}Test binary: $TEST_BINARY${NC}"
    exit 0
fi

# Print test information
echo -e "${BLUE}[INFO] Test Configuration${NC}"
echo -e "${BLUE}  Server: rs2.rainmanjam.com:443${NC}"
echo -e "${BLUE}  Protocol: HTTPS${NC}"
echo -e "${BLUE}  Test Suite: 6 comprehensive E2E tests${NC}"
echo ""

# Warning about live server
echo -e "${YELLOW}========================================================================${NC}"
echo -e "${YELLOW}  WARNING: These tests run against a LIVE Restreamer server!${NC}"
echo -e "${YELLOW}  - Tests will create/start/stop/delete processes on the server${NC}"
echo -e "${YELLOW}  - Requires network connectivity to rs2.rainmanjam.com${NC}"
echo -e "${YELLOW}  - May take several minutes to complete${NC}"
echo -e "${YELLOW}========================================================================${NC}"
echo ""

# Prompt to continue
read -p "Continue with tests? [y/N] " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${YELLOW}[CANCELLED] Tests cancelled by user${NC}"
    exit 0
fi
echo ""

# Run tests
echo -e "${YELLOW}[TEST] Running E2E streaming workflow tests...${NC}"
echo ""

if [ "$VERBOSE" = true ]; then
    # Run with CTest verbose mode
    cd "$BUILD_DIR"
    ctest -R e2e_streaming_workflow --verbose --output-on-failure
    TEST_RESULT=$?
else
    # Run test binary directly
    "$TEST_BINARY"
    TEST_RESULT=$?
fi

echo ""

# Print results
if [ $TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}========================================================================${NC}"
    echo -e "${GREEN}  ✓ All E2E streaming workflow tests PASSED${NC}"
    echo -e "${GREEN}========================================================================${NC}"
    exit 0
else
    echo -e "${RED}========================================================================${NC}"
    echo -e "${RED}  ✗ E2E streaming workflow tests FAILED${NC}"
    echo -e "${RED}========================================================================${NC}"
    echo ""
    echo -e "${YELLOW}Troubleshooting tips:${NC}"
    echo "  1. Check server connectivity: ping rs2.rainmanjam.com"
    echo "  2. Verify credentials are correct"
    echo "  3. Check server logs for errors"
    echo "  4. Run with --verbose for detailed output"
    echo "  5. See tests/e2e/STREAMING_WORKFLOW_TESTS.md for more help"
    exit 1
fi
