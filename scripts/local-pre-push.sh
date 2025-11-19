#!/bin/bash
# Quick local pre-push validation script
# Run this before pushing to catch common issues early
# This provides fast feedback (~2-3 minutes) vs ~6-8 minutes on GitHub Actions

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
echo -e "${BLUE}OBS Polyemesis - Pre-Push Validation${NC}"
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
echo

# Step 1: Check code formatting
echo -e "${YELLOW}[1/4] Checking code formatting...${NC}"
if [ -x "scripts/check-format.sh" ]; then
    ./scripts/check-format.sh || {
        echo -e "${RED}Code formatting check failed!${NC}"
        echo "Run: clang-format -i <files> to fix"
        exit 1
    }
    echo -e "${GREEN}✓ Code formatting OK${NC}"
else
    echo -e "${YELLOW}⚠ check-format.sh not found, skipping${NC}"
fi
echo

# Step 2: Verify code quality
echo -e "${YELLOW}[2/4] Running code verification...${NC}"
if [ -x "scripts/verify_code.sh" ]; then
    ./scripts/verify_code.sh || {
        echo -e "${RED}Code verification failed!${NC}"
        exit 1
    }
    echo -e "${GREEN}✓ Code verification passed${NC}"
else
    echo -e "${YELLOW}⚠ verify_code.sh not found, skipping${NC}"
fi
echo

# Step 3: Build and run unit tests (platform-specific)
echo -e "${YELLOW}[3/4] Building and running unit tests...${NC}"
if [ "$PLATFORM" = "macOS" ]; then
    if [ -x "scripts/macos-test.sh" ]; then
        ./scripts/macos-test.sh --build-first || {
            echo -e "${RED}macOS unit tests failed!${NC}"
            exit 1
        }
        echo -e "${GREEN}✓ macOS unit tests passed${NC}"
    else
        echo -e "${RED}macos-test.sh not found${NC}"
        exit 1
    fi
elif [ "$PLATFORM" = "Linux" ]; then
    # Build and test on Linux
    cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_TESTING=ON \
        -DENABLE_FRONTEND_API=OFF \
        -DENABLE_QT=OFF

    cmake --build build --target obs-polyemesis-tests

    cd build
    ctest --output-on-failure || {
        echo -e "${RED}Linux unit tests failed!${NC}"
        exit 1
    }
    cd ..
    echo -e "${GREEN}✓ Linux unit tests passed${NC}"
fi
echo

# Step 4: ShellCheck validation
echo -e "${YELLOW}[4/4] Running ShellCheck on scripts...${NC}"
if command -v shellcheck &> /dev/null; then
    shellcheck scripts/*.sh || {
        echo -e "${YELLOW}⚠ ShellCheck found issues (non-blocking)${NC}"
    }
    echo -e "${GREEN}✓ ShellCheck complete${NC}"
else
    echo -e "${YELLOW}⚠ shellcheck not installed, skipping${NC}"
    echo "  Install: brew install shellcheck (macOS) or apt-get install shellcheck (Linux)"
fi
echo

# Success summary
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}✓ All pre-push checks passed!${NC}"
echo -e "${GREEN}========================================${NC}"
echo
echo -e "${BLUE}Your code is ready to push to GitHub.${NC}"
echo -e "${BLUE}GitHub Actions will run comprehensive tests including:${NC}"
echo -e "  - Integration Tests (manual trigger)"
echo -e "  - E2E Tests (manual trigger)"
echo -e "  - Code Coverage (manual trigger)"
echo -e "  - Security Scanning (automatic)"
echo -e "  - Cross-platform builds (automatic)"
echo
