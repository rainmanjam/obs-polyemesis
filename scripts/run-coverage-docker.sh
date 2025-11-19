#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCKER_IMAGE="obs-polyemesis-coverage"
CONTAINER_NAME="obs-coverage-$$"
COVERAGE_DIR="$PROJECT_ROOT/coverage"

# Cleanup on exit
cleanup() {
    if [ -n "$CONTAINER_NAME" ]; then
        docker rm -f "$CONTAINER_NAME" 2>/dev/null || true
    fi
}
trap cleanup EXIT

log_info "OBS Polyemesis - Docker Coverage Generation"
echo "========================================"

# Check Docker
if ! command -v docker &> /dev/null; then
    log_error "Docker is not installed"
    exit 1
fi
log_success "Docker found"

# Build coverage image
log_info "Building coverage Docker image..."
if docker build -t "$DOCKER_IMAGE" -f "$PROJECT_ROOT/Dockerfile.coverage" "$PROJECT_ROOT"; then
    log_success "Docker image built: $DOCKER_IMAGE"
else
    log_error "Failed to build Docker image"
    exit 1
fi

# Create coverage output directory
mkdir -p "$COVERAGE_DIR"

# Start container
log_info "Starting coverage container..."
docker run -d \
    --name "$CONTAINER_NAME" \
    -v "$PROJECT_ROOT:/workspace" \
    -w /workspace \
    "$DOCKER_IMAGE" \
    sleep infinity

log_success "Container started: $CONTAINER_NAME"

# Generate coverage
log_info "Generating coverage data..."
if docker exec "$CONTAINER_NAME" bash -c "
    set -e

    # Clean previous build
    rm -rf build_coverage
    mkdir -p build_coverage
    cd build_coverage

    # Configure with coverage flags
    cmake .. -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_C_FLAGS='--coverage -fprofile-arcs -ftest-coverage' \
        -DCMAKE_EXE_LINKER_FLAGS='--coverage' \
        -DENABLE_FRONTEND_API=OFF \
        -DENABLE_QT=OFF \
        -DENABLE_TESTING=ON

    # Build tests
    cmake --build . --target obs-polyemesis-tests

    # Run tests to generate coverage data
    cd tests
    ./obs-polyemesis-tests

    cd /workspace

    # Generate coverage report
    lcov --capture \
        --directory build_coverage \
        --output-file coverage/coverage.info \
        --rc lcov_branch_coverage=1

    # Remove system/external files
    lcov --remove coverage/coverage.info \
        '/usr/*' \
        '*/tests/*' \
        --output-file coverage/coverage_filtered.info \
        --rc lcov_branch_coverage=1

    # Generate HTML report
    genhtml coverage/coverage_filtered.info \
        --output-directory coverage/html \
        --title 'OBS Polyemesis Coverage' \
        --num-spaces 2 \
        --sort \
        --function-coverage \
        --branch-coverage \
        --rc lcov_branch_coverage=1

    # Generate summary
    lcov --summary coverage/coverage_filtered.info \
        --rc lcov_branch_coverage=1 > coverage/summary.txt 2>&1

    # Generate badge (if installed)
    if command -v genbadge &> /dev/null; then
        genbadge coverage -i coverage/coverage_filtered.info -o coverage/coverage-badge.svg 2>/dev/null || true
    fi

    echo 'Coverage generation complete!'
"; then
    log_success "Coverage generated successfully"
else
    log_error "Coverage generation failed"
    exit 1
fi

# Show summary
log_info "Coverage Summary:"
if [ -f "$COVERAGE_DIR/summary.txt" ]; then
    cat "$COVERAGE_DIR/summary.txt"
fi

# Provide report location
log_success "Coverage report generated!"
echo ""
echo "HTML Report: file://$COVERAGE_DIR/html/index.html"
echo "Coverage Data: $COVERAGE_DIR/coverage_filtered.info"
echo "Summary: $COVERAGE_DIR/summary.txt"
echo ""
log_info "To view report, open: $COVERAGE_DIR/html/index.html"

# Cleanup
log_info "Cleaning up container..."
docker rm -f "$CONTAINER_NAME"
log_success "Coverage generation complete!"
