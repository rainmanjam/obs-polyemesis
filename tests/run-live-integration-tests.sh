#!/bin/bash
#
# Run live integration tests against Restreamer server
#
# This script runs integration tests against a live Restreamer server.
# Tests include authentication, process management, and error handling.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}Live Restreamer API Integration Tests${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found at $BUILD_DIR${NC}"
    echo "Please run 'cmake -B build' first to configure the project."
    exit 1
fi

# Check if test executable exists
TEST_EXECUTABLE="$BUILD_DIR/tests/test_restreamer_api_integration"

if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${YELLOW}Test executable not found. Building...${NC}"
    cd "$BUILD_DIR"
    cmake --build . --target test_restreamer_api_integration

    if [ ! -f "$TEST_EXECUTABLE" ]; then
        echo -e "${RED}Error: Failed to build test executable${NC}"
        exit 1
    fi
fi

echo -e "${GREEN}Test executable found at:${NC}"
echo "  $TEST_EXECUTABLE"
echo ""

# Display test information
echo -e "${YELLOW}Test Configuration:${NC}"
echo "  Server: https://rs2.rainmanjam.com"
echo "  Port: 443"
echo "  Tests: 9 comprehensive integration tests"
echo "  Labels: integration, network, live-server"
echo ""

# Ask for confirmation
echo -e "${YELLOW}Note:${NC} These tests will connect to a live server and may:"
echo "  - Create and delete test processes"
echo "  - Use network bandwidth"
echo "  - Take several seconds to complete"
echo ""

read -p "Continue with tests? [Y/n] " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]] && [[ ! -z $REPLY ]]; then
    echo -e "${YELLOW}Tests cancelled.${NC}"
    exit 0
fi

echo ""
echo -e "${BLUE}Running integration tests...${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""

# Run the tests
cd "$BUILD_DIR"

if "$TEST_EXECUTABLE"; then
    echo ""
    echo -e "${BLUE}================================================${NC}"
    echo -e "${GREEN}✓ All integration tests passed!${NC}"
    echo -e "${BLUE}================================================${NC}"
    exit 0
else
    EXIT_CODE=$?
    echo ""
    echo -e "${BLUE}================================================${NC}"
    echo -e "${RED}✗ Some integration tests failed!${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo ""
    echo -e "${YELLOW}Troubleshooting tips:${NC}"
    echo "  1. Check server connectivity: curl -k https://rs2.rainmanjam.com/api/v3/"
    echo "  2. Verify credentials are correct"
    echo "  3. Check firewall settings"
    echo "  4. Review server logs for errors"
    echo ""
    exit $EXIT_CODE
fi
