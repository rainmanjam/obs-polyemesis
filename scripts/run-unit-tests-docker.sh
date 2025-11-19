#!/bin/bash
# OBS Polyemesis - Docker Unit Test Runner
# Runs C++ unit tests in isolated Docker environment to prevent port conflicts

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
DOCKER_IMAGE="obs-polyemesis-test-runner"
CONTAINER_NAME="obs-polyemesis-tests-$$"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

cleanup() {
    if [ -n "$CONTAINER_NAME" ]; then
        log_info "Cleaning up container..."
        docker rm -f "$CONTAINER_NAME" 2>/dev/null || true
    fi
}

trap cleanup EXIT

# Main script
log_info "OBS Polyemesis - Docker Unit Test Runner"
echo "========================================"

# Check Docker
if ! command -v docker &> /dev/null; then
    log_error "Docker not found. Please install Docker first."
    exit 1
fi

log_success "Docker found"

# Build Docker image
log_info "Building test runner Docker image..."
if docker build -t "$DOCKER_IMAGE" -f "$PROJECT_ROOT/Dockerfile.test-runner" "$PROJECT_ROOT"; then
    log_success "Docker image built: $DOCKER_IMAGE"
else
    log_error "Failed to build Docker image"
    exit 1
fi

# Create and start container
log_info "Starting test container..."
docker run -d \
    --name "$CONTAINER_NAME" \
    -v "$PROJECT_ROOT:/workspace" \
    -w /workspace \
    "$DOCKER_IMAGE" \
    sleep infinity

log_success "Container started: $CONTAINER_NAME"

# Build tests inside container
log_info "Building tests inside container..."
if docker exec "$CONTAINER_NAME" bash -c "
    set -e
    mkdir -p build_docker_tests
    cd build_docker_tests
    cmake .. -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_FRONTEND_API=OFF \
        -DENABLE_QT=OFF \
        -DENABLE_TESTING=ON
    cmake --build . --target obs-polyemesis-tests
"; then
    log_success "Tests built successfully"
else
    log_error "Test build failed"
    exit 1
fi

# Run tests inside container
log_info "Running unit tests in isolated environment..."
echo "========================================"

# Save test output to a file and display it
if docker exec "$CONTAINER_NAME" bash -c "
    cd build_docker_tests/tests
    ./obs-polyemesis-tests 2>&1 | tee test_output.log
    exit \${PIPESTATUS[0]}
"; then
    TEST_EXIT_CODE=0
    log_success "All tests passed!"
else
    TEST_EXIT_CODE=$?
    if [ $TEST_EXIT_CODE -eq 139 ]; then
        log_error "Tests crashed with SIGSEGV (exit code 139)"
        log_info "Attempting to get test summary from output..."
        docker exec "$CONTAINER_NAME" bash -c "
            cd build_docker_tests/tests
            grep -E '(Passed:|Failed:|Total:|✓|✗)' test_output.log | tail -20 || echo 'Could not extract summary'
        "
    else
        log_warning "Some tests failed (exit code: $TEST_EXIT_CODE)"
    fi
fi

# Summary
echo ""
echo "========================================"
if [ $TEST_EXIT_CODE -eq 0 ]; then
    log_success "Docker unit tests completed successfully!"
else
    log_warning "Docker unit tests completed with failures"
fi
echo "Container: $CONTAINER_NAME (will be cleaned up)"
echo ""
log_info "Benefits of Docker testing:"
echo "  ✓ Isolated network namespace (no port conflicts)"
echo "  ✓ Clean environment for each run"
echo "  ✓ Reproducible test conditions"
echo "  ✓ Automatic cleanup"
echo ""

exit $TEST_EXIT_CODE
