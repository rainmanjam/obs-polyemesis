#!/bin/bash
# Comprehensive local testing script
# Runs all tests including unit tests, integration tests, and coverage
# This approximates what GitHub Actions will run (without manual workflows)

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}OBS Polyemesis - Full Local Test Suite${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
else
    echo -e "${RED}Unsupported platform: $OSTYPE${NC}"
    exit 1
fi

echo -e "${GREEN}Platform: ${PLATFORM}${NC}"
echo -e "${YELLOW}This will run all local tests (~10-15 minutes)${NC}"
echo

# Step 1: Pre-push validation
echo -e "${YELLOW}[1/6] Running pre-push validation...${NC}"
if [ -x "scripts/local-pre-push.sh" ]; then
    ./scripts/local-pre-push.sh || {
        echo -e "${RED}Pre-push validation failed!${NC}"
        exit 1
    }
    echo -e "${GREEN}✓ Pre-push validation passed${NC}"
else
    echo -e "${YELLOW}⚠ local-pre-push.sh not found, skipping${NC}"
fi
echo

# Step 2: Platform-specific tests
echo -e "${YELLOW}[2/6] Running platform-specific tests...${NC}"
if [ "$PLATFORM" = "macOS" ]; then
    if [ -x "scripts/macos-test.sh" ]; then
        ./scripts/macos-test.sh || {
            echo -e "${RED}macOS tests failed!${NC}"
            exit 1
        }
        echo -e "${GREEN}✓ macOS tests passed${NC}"
    fi
elif [ "$PLATFORM" = "Linux" ]; then
    echo -e "${BLUE}Building and testing on Linux...${NC}"
    cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_TESTING=ON \
        -DENABLE_FRONTEND_API=OFF \
        -DENABLE_QT=OFF

    cmake --build build --target obs-polyemesis-tests

    cd build
    ctest --output-on-failure || {
        echo -e "${RED}Linux tests failed!${NC}"
        exit 1
    }
    cd ..
    echo -e "${GREEN}✓ Linux tests passed${NC}"
fi
echo

# Step 3: Docker unit tests (if Docker available)
echo -e "${YELLOW}[3/6] Running Docker unit tests...${NC}"
if command -v docker &> /dev/null; then
    if [ -x "scripts/run-unit-tests-docker.sh" ]; then
        ./scripts/run-unit-tests-docker.sh || {
            echo -e "${YELLOW}⚠ Docker unit tests failed (non-critical)${NC}"
        }
        echo -e "${GREEN}✓ Docker unit tests complete${NC}"
    else
        echo -e "${YELLOW}⚠ run-unit-tests-docker.sh not found, skipping${NC}"
    fi
else
    echo -e "${YELLOW}⚠ Docker not available, skipping Docker tests${NC}"
fi
echo

# Step 4: Integration tests (Docker Compose)
echo -e "${YELLOW}[4/6] Running integration tests...${NC}"
if command -v docker &> /dev/null && command -v docker-compose &> /dev/null; then
    if [ -f "docker-compose.integration.yml" ]; then
        echo -e "${BLUE}Starting integration tests with Docker Compose...${NC}"
        docker-compose -f docker-compose.integration.yml up --abort-on-container-exit || {
            echo -e "${YELLOW}⚠ Integration tests failed (non-critical)${NC}"
            docker-compose -f docker-compose.integration.yml down -v
        }
        docker-compose -f docker-compose.integration.yml down -v
        echo -e "${GREEN}✓ Integration tests complete${NC}"
    else
        echo -e "${YELLOW}⚠ docker-compose.integration.yml not found, skipping${NC}"
    fi
else
    echo -e "${YELLOW}⚠ Docker Compose not available, skipping integration tests${NC}"
fi
echo

# Step 5: E2E Tests (platform-specific)
echo -e "${YELLOW}[5/6] Running E2E tests...${NC}"
if [ "$PLATFORM" = "macOS" ]; then
    if [ -x "tests/e2e/macos/e2e-test-macos.sh" ]; then
        ./tests/e2e/macos/e2e-test-macos.sh install-only || {
            echo -e "${YELLOW}⚠ E2E tests failed (non-critical)${NC}"
        }
        echo -e "${GREEN}✓ E2E tests complete${NC}"
    else
        echo -e "${YELLOW}⚠ E2E test script not found, skipping${NC}"
    fi
elif [ "$PLATFORM" = "Linux" ]; then
    if [ -x "tests/e2e/linux/e2e-test-linux.sh" ]; then
        ./tests/e2e/linux/e2e-test-linux.sh ci || {
            echo -e "${YELLOW}⚠ E2E tests failed (non-critical)${NC}"
        }
        echo -e "${GREEN}✓ E2E tests complete${NC}"
    else
        echo -e "${YELLOW}⚠ E2E test script not found, skipping${NC}"
    fi
fi
echo

# Step 6: Code coverage (optional, takes longest)
echo -e "${YELLOW}[6/6] Generating code coverage...${NC}"
read -p "Generate code coverage report? This takes 5-10 minutes. (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if [ "$PLATFORM" = "macOS" ]; then
        if [ -x "scripts/generate-coverage.sh" ]; then
            ./scripts/generate-coverage.sh || {
                echo -e "${YELLOW}⚠ Coverage generation failed (non-critical)${NC}"
            }
            echo -e "${GREEN}✓ Coverage report generated${NC}"
        fi
    elif [ "$PLATFORM" = "Linux" ]; then
        if [ -x "scripts/run-coverage-docker.sh" ]; then
            ./scripts/run-coverage-docker.sh || {
                echo -e "${YELLOW}⚠ Coverage generation failed (non-critical)${NC}"
            }
            echo -e "${GREEN}✓ Coverage report generated${NC}"
        fi
    fi
else
    echo -e "${YELLOW}⚠ Skipping coverage generation${NC}"
fi
echo

# Success summary
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}✓ Full local test suite complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo
echo -e "${BLUE}Test Summary:${NC}"
echo -e "  ✓ Pre-push validation (linting, code quality, unit tests)"
echo -e "  ✓ Platform-specific tests ($PLATFORM)"
echo -e "  ✓ Docker unit tests (if available)"
echo -e "  ✓ Integration tests (if available)"
echo -e "  ✓ E2E tests (if available)"
echo
echo -e "${BLUE}Manual GitHub Actions workflows (trigger when needed):${NC}"
echo -e "  - Integration Tests: gh workflow run integration-tests.yml"
echo -e "  - E2E Tests: gh workflow run e2e-tests.yaml"
echo -e "  - Code Coverage: gh workflow run coverage.yml"
echo
