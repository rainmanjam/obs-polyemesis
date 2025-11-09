#!/bin/bash
# Code style verification script for OBS Polyemesis
# Checks CMake and C/C++ code formatting

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=================================="
echo "OBS Polyemesis Code Style Checker"
echo "=================================="
echo ""

ERRORS=0

# Check if gersemi is installed
if ! command -v gersemi &> /dev/null; then
    echo -e "${RED}ERROR: gersemi not found${NC}"
    echo "Install with: pip3 install gersemi"
    exit 1
fi

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo -e "${YELLOW}WARNING: clang-format not found${NC}"
    echo "Install with: brew install clang-format (macOS) or apt install clang-format (Linux)"
    SKIP_CLANG_FORMAT=1
else
    SKIP_CLANG_FORMAT=0
fi

# Check CMake formatting with gersemi
echo -e "${YELLOW}Checking CMake formatting...${NC}"
if gersemi --check CMakeLists.txt cmake/**/*.cmake 2>&1 | grep -q "requires formatting"; then
    echo -e "${RED}✖ CMake files need formatting${NC}"
    echo ""
    echo "Files that need formatting:"
    gersemi --check CMakeLists.txt cmake/**/*.cmake 2>&1 | grep "requires formatting" | sed 's/.*   ✖︎   /  - /'
    echo ""
    echo "To fix automatically, run:"
    echo "  gersemi --in-place CMakeLists.txt cmake/**/*.cmake"
    echo ""
    ERRORS=$((ERRORS + 1))
else
    echo -e "${GREEN}✓ CMake formatting is correct${NC}"
fi

# Check C/C++ formatting with clang-format
if [ $SKIP_CLANG_FORMAT -eq 0 ]; then
    echo -e "${YELLOW}Checking C/C++ formatting...${NC}"

    # Find all C/C++ source files
    FILES=$(find src -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \))

    FORMAT_ERRORS=0
    for file in $FILES; do
        if ! clang-format --dry-run --Werror "$file" &> /dev/null; then
            if [ $FORMAT_ERRORS -eq 0 ]; then
                echo -e "${RED}✖ C/C++ files need formatting${NC}"
                echo ""
                echo "Files that need formatting:"
            fi
            echo "  - $file"
            FORMAT_ERRORS=$((FORMAT_ERRORS + 1))
        fi
    done

    if [ $FORMAT_ERRORS -gt 0 ]; then
        echo ""
        echo "To fix automatically, run:"
        echo "  clang-format -i src/*.c src/*.cpp src/*.h"
        echo ""
        ERRORS=$((ERRORS + 1))
    else
        echo -e "${GREEN}✓ C/C++ formatting is correct${NC}"
    fi
fi

echo ""
echo "=================================="
if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✓ All code style checks passed!${NC}"
    exit 0
else
    echo -e "${RED}✖ Found $ERRORS formatting issue(s)${NC}"
    echo "Please fix the issues above before committing."
    exit 1
fi
