#!/bin/bash
# Comprehensive Test Runner for OBS Polyemesis
# Runs all test suites: Integration, Error Handling, E2E Workflows

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
RUN_INTEGRATION=${RUN_INTEGRATION:-true}
RUN_ERROR_HANDLING=${RUN_ERROR_HANDLING:-true}
RUN_E2E=${RUN_E2E:-true}
RUN_UNIT=${RUN_UNIT:-false}  # Disabled by default (requires build)

RESTREAMER_HOST="${RESTREAMER_HOST:-localhost}"
RESTREAMER_PORT="${RESTREAMER_PORT:-8080}"

# Test results tracking
TOTAL_SUITES=0
PASSED_SUITES=0
FAILED_SUITES=0

echo "========================================================================"
echo -e "${CYAN}       OBS POLYEMESIS - COMPREHENSIVE TEST SUITE${NC}"
echo "========================================================================"
echo ""
echo -e "${BLUE}Configuration:${NC}"
echo "  Restreamer: http://${RESTREAMER_HOST}:${RESTREAMER_PORT}"
echo "  Integration Tests: ${RUN_INTEGRATION}"
echo "  Error Handling Tests: ${RUN_ERROR_HANDLING}"
echo "  E2E Workflow Tests: ${RUN_E2E}"
echo "  Unit Tests: ${RUN_UNIT}"
echo ""

# Check if Docker environment is needed
if [ "$RUN_INTEGRATION" = true ] || [ "$RUN_ERROR_HANDLING" = true ] || [ "$RUN_E2E" = true ]; then
    echo -e "${YELLOW}[INFO]${NC} Checking Docker test environment..."

    # Check if Restreamer is running
    if ! curl -s -f -o /dev/null "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/v3/about" 2>/dev/null && \
       ! curl -s -o /dev/null -w "%{http_code}" "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/v3/about" 2>/dev/null | grep -q "401"; then
        echo -e "${YELLOW}[WARN]${NC} Restreamer not running. Starting Docker test environment..."
        docker compose -f docker-compose.test.yml up -d

        # Wait for services
        echo -e "${YELLOW}[INFO]${NC} Waiting for services to be ready..."
        sleep 10
    else
        echo -e "${GREEN}[INFO]${NC} Restreamer is already running"
    fi
fi

# Run Integration Tests
if [ "$RUN_INTEGRATION" = true ]; then
    TOTAL_SUITES=$((TOTAL_SUITES + 1))
    echo ""
    echo "========================================================================"
    echo -e "${MAGENTA}TEST SUITE 1: API INTEGRATION TESTS${NC}"
    echo "========================================================================"

    if ./tests/api_integration_test.sh; then
        PASSED_SUITES=$((PASSED_SUITES + 1))
        echo -e "${GREEN}✓ Integration Tests PASSED${NC}"
    else
        FAILED_SUITES=$((FAILED_SUITES + 1))
        echo -e "${RED}✗ Integration Tests FAILED${NC}"
    fi
fi

# Run Error Handling Tests
if [ "$RUN_ERROR_HANDLING" = true ]; then
    TOTAL_SUITES=$((TOTAL_SUITES + 1))
    echo ""
    echo "========================================================================"
    echo -e "${MAGENTA}TEST SUITE 2: ERROR HANDLING & EDGE CASE TESTS${NC}"
    echo "========================================================================"

    if ./tests/api_error_handling_test.sh; then
        PASSED_SUITES=$((PASSED_SUITES + 1))
        echo -e "${GREEN}✓ Error Handling Tests PASSED${NC}"
    else
        FAILED_SUITES=$((FAILED_SUITES + 1))
        echo -e "${RED}✗ Error Handling Tests FAILED${NC}"
    fi
fi

# Run E2E Workflow Tests
if [ "$RUN_E2E" = true ]; then
    TOTAL_SUITES=$((TOTAL_SUITES + 1))
    echo ""
    echo "========================================================================"
    echo -e "${MAGENTA}TEST SUITE 3: END-TO-END WORKFLOW TESTS${NC}"
    echo "========================================================================"

    if ./tests/e2e_workflow_test.sh; then
        PASSED_SUITES=$((PASSED_SUITES + 1))
        echo -e "${GREEN}✓ E2E Workflow Tests PASSED${NC}"
    else
        FAILED_SUITES=$((FAILED_SUITES + 1))
        echo -e "${RED}✗ E2E Workflow Tests FAILED${NC}"
    fi
fi

# Run Unit Tests (if enabled)
if [ "$RUN_UNIT" = true ]; then
    TOTAL_SUITES=$((TOTAL_SUITES + 1))
    echo ""
    echo "========================================================================"
    echo -e "${MAGENTA}TEST SUITE 4: UNIT TESTS (C++)${NC}"
    echo "========================================================================"

    if [ -d "build/tests" ]; then
        if cd build/tests && ctest --output-on-failure; then
            cd ../..
            PASSED_SUITES=$((PASSED_SUITES + 1))
            echo -e "${GREEN}✓ Unit Tests PASSED${NC}"
        else
            cd ../..
            FAILED_SUITES=$((FAILED_SUITES + 1))
            echo -e "${RED}✗ Unit Tests FAILED${NC}"
        fi
    else
        echo -e "${YELLOW}[SKIP]${NC} Unit tests not built. Run 'cmake -B build && cmake --build build' first."
        TOTAL_SUITES=$((TOTAL_SUITES - 1))
    fi
fi

# Print comprehensive summary
echo ""
echo "========================================================================"
echo -e "${CYAN}                    COMPREHENSIVE TEST SUMMARY${NC}"
echo "========================================================================"
echo ""
echo "Test Suites Executed: $TOTAL_SUITES"
echo -e "Passed:               ${GREEN}${PASSED_SUITES}${NC}"
echo -e "Failed:               ${RED}${FAILED_SUITES}${NC}"
echo ""

if [ $FAILED_SUITES -eq 0 ]; then
    echo -e "${GREEN}╔════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║  ALL TEST SUITES PASSED SUCCESSFULLY  ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════╝${NC}"
    echo ""

    # Optional: Cleanup Docker environment
    if [ "${CLEANUP_DOCKER:-false}" = true ]; then
        echo -e "${YELLOW}[INFO]${NC} Cleaning up Docker test environment..."
        docker compose -f docker-compose.test.yml down -v
    fi

    exit 0
else
    echo -e "${RED}╔════════════════════════════════════════╗${NC}"
    echo -e "${RED}║    SOME TEST SUITES FAILED             ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════╝${NC}"
    echo ""
    echo "Review the output above for details on failures."
    echo ""

    exit 1
fi
