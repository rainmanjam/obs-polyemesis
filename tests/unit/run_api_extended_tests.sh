#!/bin/bash
# Script to build and run extended API unit tests

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

echo -e "${YELLOW}=== Building Extended API Tests ===${NC}"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure CMake with testing enabled
echo "Configuring CMake..."
cmake .. -DENABLE_TESTING=ON \
    -DENABLE_FRONTEND_API=ON \
    -DENABLE_QT=ON \
    || { echo -e "${RED}CMake configuration failed${NC}"; exit 1; }

# Build the extended API test executable
echo "Building test_api_extended..."
cmake --build . --target test_api_extended -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4) \
    || { echo -e "${RED}Build failed${NC}"; exit 1; }

echo -e "${GREEN}Build successful!${NC}"
echo ""
echo -e "${YELLOW}=== Running Extended API Tests ===${NC}"
echo ""

# Run the tests
if [ -f "${BUILD_DIR}/tests/test_api_extended" ]; then
    "${BUILD_DIR}/tests/test_api_extended"
    TEST_RESULT=$?

    echo ""
    if [ $TEST_RESULT -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
    else
        echo -e "${RED}Some tests failed (exit code: $TEST_RESULT)${NC}"
    fi

    exit $TEST_RESULT
else
    echo -e "${RED}Test executable not found at: ${BUILD_DIR}/tests/test_api_extended${NC}"
    exit 1
fi
