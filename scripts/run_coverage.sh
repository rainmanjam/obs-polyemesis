#!/bin/bash
# Code Coverage Analysis Script

set -e

echo "=== Code Coverage Analysis ==="

# Navigate to project root
cd "$(dirname "$0")/.."

# Clean and create coverage build
rm -rf build-coverage
mkdir build-coverage
cd build-coverage

# Configure with coverage (using Xcode generator required for macOS)
cmake .. \
  -G Xcode \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_COVERAGE=ON \
  -DENABLE_TESTING=ON \
  -DCMAKE_C_FLAGS="--coverage -O0 -g" \
  -DCMAKE_CXX_FLAGS="--coverage -O0 -g" \
  -DCMAKE_EXE_LINKER_FLAGS="--coverage"

# Build tests
echo "Building tests with coverage..."
cmake --build . --config Debug --target test_profile_management_standalone test_failover_standalone test_edge_cases_standalone test_platform_compat_standalone 2>&1 | grep -E "(Building|Linking|error|warning|Compiling)" | tail -20

# Run tests
echo ""
echo "Running tests..."
for test in tests/Debug/test_profile_management_standalone tests/Debug/test_failover_standalone tests/Debug/test_edge_cases_standalone tests/Debug/test_platform_compat_standalone; do
    if [ -f "$test" ]; then
        echo "Executing: $test"
        ./$test || echo "⚠️  Test failed but continuing..."
    fi
done

# Generate coverage data
echo ""
echo "Generating coverage data..."
lcov --capture \
     --directory . \
     --output-file coverage.info \
     --ignore-errors mismatch,gcov,source \
     --rc lcov_branch_coverage=1 2>&1 | grep -v "geninfo: WARNING"

# Filter out system headers and test files
lcov --remove coverage.info \
     '/usr/*' \
     '/Applications/*' \
     '*/tests/*' \
     '*/obs_stubs.c' \
     '*/mock_*' \
     '*/Qt*' \
     --output-file coverage_filtered.info \
     --ignore-errors source

# Generate HTML report
genhtml coverage_filtered.info \
        --output-directory coverage_html \
        --title "OBS Polyemesis Coverage Report" \
        --legend \
        --branch-coverage \
        --demangle-cpp \
        2>&1 | grep -v "genhtml: WARNING"

# Print summary
echo ""
echo "=== Coverage Summary ==="
lcov --list coverage_filtered.info | grep -E "(Total:|src/)" | grep -v "test"

# Extract percentage
COVERAGE=$(lcov --list coverage_filtered.info | grep "Total:" | awk '{print $2}')

echo ""
echo "===================="
echo "Overall Coverage: $COVERAGE"
echo "===================="
echo ""
echo "Coverage report: build-coverage/coverage_html/index.html"
echo "View with: open build-coverage/coverage_html/index.html"

# Update TESTING_SUMMARY.md
cd ..
if [ -f TESTING_SUMMARY.md ]; then
    if ! grep -q "Code Coverage" TESTING_SUMMARY.md; then
        cat >> TESTING_SUMMARY.md << EOF

---

## Code Coverage

**Coverage:** $COVERAGE
**Report:** \`build-coverage/coverage_html/index.html\`
**Last Updated:** $(date +%Y-%m-%d)

Generated with lcov/genhtml. Coverage includes:
- restreamer-api.c
- restreamer-output-profile.c
- restreamer-multistream.c
- restreamer-config.c
- restreamer-source.c
- restreamer-output.c

EOF
        echo "✅ Updated TESTING_SUMMARY.md with coverage"
    fi
fi

echo ""
echo "✅ Coverage analysis complete!"
