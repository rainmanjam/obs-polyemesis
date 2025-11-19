#!/bin/bash
# Generate code coverage reports for OBS Polyemesis
# Uses lcov to generate HTML coverage reports from test execution

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build_coverage}"
COVERAGE_DIR="${BUILD_DIR}/coverage"
COVERAGE_HTML="${COVERAGE_DIR}/html"

echo -e "${GREEN}========================================"
echo "OBS Polyemesis - Coverage Report Generator"
echo -e "========================================${NC}"
echo

# Check for lcov
if ! command -v lcov &> /dev/null; then
    echo -e "${RED}Error: lcov not found${NC}"
    echo "Install with: brew install lcov"
    exit 1
fi

# Check for genhtml
if ! command -v genhtml &> /dev/null; then
    echo -e "${RED}Error: genhtml not found${NC}"
    echo "Install with: brew install lcov"
    exit 1
fi

echo -e "${YELLOW}[1/6] Cleaning previous coverage data...${NC}"
rm -rf "${COVERAGE_DIR}"
mkdir -p "${COVERAGE_DIR}"
mkdir -p "${COVERAGE_HTML}"

echo -e "${YELLOW}[2/6] Configuring build with coverage enabled...${NC}"
cmake -B "${BUILD_DIR}" -G Xcode \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_TESTING=ON \
    -DENABLE_COVERAGE=ON \
    -DENABLE_FRONTEND_API=OFF \
    -DENABLE_QT=OFF

echo -e "${YELLOW}[3/6] Building tests with coverage instrumentation...${NC}"
cmake --build "${BUILD_DIR}" --config Debug --target obs-polyemesis-tests

echo -e "${YELLOW}[4/6] Running tests to generate coverage data...${NC}"
cd "${BUILD_DIR}"
# Run tests (ignore failures for coverage purposes)
# Note: Xcode generator requires -C Debug configuration flag
ctest -C Debug --output-on-failure || {
    echo -e "${YELLOW}Warning: Some tests failed, but continuing with coverage generation${NC}"
}
cd "${PROJECT_ROOT}"

echo -e "${YELLOW}[5/6] Capturing coverage data...${NC}"
# Capture initial baseline
lcov --capture --initial \
    --directory "${BUILD_DIR}" \
    --output-file "${COVERAGE_DIR}/base.info" \
    --rc lcov_branch_coverage=1

# Capture test coverage
lcov --capture \
    --directory "${BUILD_DIR}" \
    --output-file "${COVERAGE_DIR}/test.info" \
    --rc lcov_branch_coverage=1

# Combine baseline and test coverage
lcov --add-tracefile "${COVERAGE_DIR}/base.info" \
    --add-tracefile "${COVERAGE_DIR}/test.info" \
    --output-file "${COVERAGE_DIR}/total.info" \
    --rc lcov_branch_coverage=1

# Filter out unwanted files (system headers, tests, dependencies)
lcov --remove "${COVERAGE_DIR}/total.info" \
    '/usr/*' \
    '/Applications/*' \
    '*/tests/*' \
    '*/test_*' \
    '*/jansson/*' \
    '*/curl/*' \
    '*/Qt/*' \
    '*/build*/*' \
    --output-file "${COVERAGE_DIR}/filtered.info" \
    --rc lcov_branch_coverage=1

echo -e "${YELLOW}[6/6] Generating HTML report...${NC}"
genhtml "${COVERAGE_DIR}/filtered.info" \
    --output-directory "${COVERAGE_HTML}" \
    --title "OBS Polyemesis Code Coverage" \
    --legend \
    --show-details \
    --branch-coverage \
    --rc genhtml_hi_limit=80 \
    --rc genhtml_med_limit=50

echo
echo -e "${GREEN}========================================"
echo "Coverage Report Generated Successfully!"
echo -e "========================================${NC}"
echo
echo -e "${GREEN}Coverage Summary:${NC}"
lcov --summary "${COVERAGE_DIR}/filtered.info" --rc lcov_branch_coverage=1
echo
echo -e "${GREEN}HTML Report Location:${NC}"
echo "  ${COVERAGE_HTML}/index.html"
echo
echo -e "${GREEN}To view the report:${NC}"
echo "  open ${COVERAGE_HTML}/index.html"
echo
