#!/bin/bash

# Comprehensive Test Runner for obs-polyemesis
# Runs all tests with sanitizers, memory leak detection, and crash tracing

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
CRASHED_TESTS=0

echo -e "${BLUE}================================================================================${NC}"
echo -e "${MAGENTA}OBS Polyemesis - Comprehensive Test Suite${NC}"
echo -e "${BLUE}================================================================================${NC}"
echo ""

# Parse arguments
ENABLE_ASAN=OFF
ENABLE_UBSAN=OFF
ENABLE_VALGRIND=OFF
BUILD_DIR="build-tests"

while [[ $# -gt 0 ]]; do
    case $1 in
        --asan)
            ENABLE_ASAN=ON
            shift
            ;;
        --ubsan)
            ENABLE_UBSAN=ON
            shift
            ;;
        --valgrind)
            ENABLE_VALGRIND=ON
            shift
            ;;
        --all-sanitizers)
            ENABLE_ASAN=ON
            ENABLE_UBSAN=ON
            shift
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Usage: $0 [--asan] [--ubsan] [--valgrind] [--all-sanitizers] [--build-dir DIR]"
            exit 1
            ;;
    esac
done

# Display configuration
echo -e "${CYAN}Test Configuration:${NC}"
echo -e "  Build Directory: $BUILD_DIR"
echo -e "  AddressSanitizer: $ENABLE_ASAN"
echo -e "  UBSan: $ENABLE_UBSAN"
echo -e "  Valgrind: $ENABLE_VALGRIND"
echo ""

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${CYAN}Creating test build directory...${NC}"
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure CMake with sanitizers
echo -e "${CYAN}Configuring CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_ASAN=$ENABLE_ASAN \
    -DENABLE_UBSAN=$ENABLE_UBSAN \
    -DENABLE_TESTING=ON \
    -G Xcode \
    > cmake_output.log 2>&1

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    cat cmake_output.log
    exit 1
fi

echo -e "${GREEN}✓ CMake configured${NC}"

# Build tests
echo -e "${CYAN}Building tests...${NC}"
# Use ALL_BUILD for Xcode generator, 'all' for others
cmake --build . --config Debug > build_output.log 2>&1

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    tail -50 build_output.log
    exit 1
fi

echo -e "${GREEN}✓ Build successful${NC}"
echo ""

# Build test subdirectory if it exists
if [ -d "tests" ]; then
    echo -e "${CYAN}Building test executables...${NC}"
    cd tests
    cmake --build . --config Debug > test_build.log 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${YELLOW}Warning: Test build had issues${NC}"
        tail -20 test_build.log
    else
        echo -e "${GREEN}✓ Test executables built${NC}"
    fi
    cd ..
fi

echo ""

# Function to run a test
run_test() {
    local test_name=$1
    local test_cmd=$2

    echo -e "${CYAN}[TEST]${NC} Running: $test_name"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    if [ "$ENABLE_VALGRIND" = "ON" ] && command -v valgrind &> /dev/null; then
        # Run with Valgrind
        valgrind --leak-check=full --show-leak-kinds=all \
                 --error-exitcode=1 --quiet \
                 $test_cmd > "${test_name}_valgrind.log" 2>&1
        local exit_code=$?
    else
        # Run normally
        $test_cmd > "${test_name}.log" 2>&1
        local exit_code=$?
    fi

    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}[PASS]${NC} $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    elif [ $exit_code -gt 128 ]; then
        # Crashed (signal)
        echo -e "${RED}[CRASH]${NC} $test_name (signal $((exit_code - 128)))"
        CRASHED_TESTS=$((CRASHED_TESTS + 1))
        tail -20 "${test_name}.log"
    else
        echo -e "${RED}[FAIL]${NC} $test_name (exit code: $exit_code)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        tail -20 "${test_name}.log"
    fi

    echo ""
}

# Run CTest
echo -e "${CYAN}Running CTest suite...${NC}"
echo ""

ctest --output-on-failure -C Debug > ctest_output.log 2>&1
ctest_result=$?

if [ $ctest_result -eq 0 ]; then
    echo -e "${GREEN}✓ CTest passed${NC}"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "${RED}✗ CTest failed${NC}"
    cat ctest_output.log
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

TOTAL_TESTS=$((TOTAL_TESTS + 1))
echo ""

# Run standalone crash-safe tests if they exist
if [ -f "tests/Debug/test_profile_management_standalone" ]; then
    run_test "profile_management" "./tests/Debug/test_profile_management_standalone"
fi

if [ -f "tests/Debug/test_failover_standalone" ]; then
    run_test "failover_logic" "./tests/Debug/test_failover_standalone"
fi

# Print summary
echo ""
echo -e "${BLUE}================================================================================${NC}"
echo -e "${MAGENTA}TEST SUMMARY${NC}"
echo -e "${BLUE}================================================================================${NC}"
echo -e "Total:   $TOTAL_TESTS"
echo -e "${GREEN}Passed:  $PASSED_TESTS${NC}"
echo -e "${RED}Failed:  $FAILED_TESTS${NC}"
echo -e "${RED}Crashed: $CRASHED_TESTS${NC}"
echo -e "${BLUE}================================================================================${NC}"

if [ $FAILED_TESTS -gt 0 ] || [ $CRASHED_TESTS -gt 0 ]; then
    echo -e "${RED}Result: FAILED${NC}"
    echo -e "${BLUE}================================================================================${NC}"
    exit 1
else
    echo -e "${GREEN}Result: PASSED${NC}"
    echo -e "${BLUE}================================================================================${NC}"
    exit 0
fi
