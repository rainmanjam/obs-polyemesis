#!/bin/bash
# OBS Polyemesis - Restreamer Integration Test Runner
# Orchestrates Docker setup, test execution, and cleanup

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
DOCKER_CONTAINER="restreamer-test"
RESTREAMER_PORT=8080
RTMP_PORT=1935
RESTREAMER_IMAGE="${RESTREAMER_IMAGE:-datarhei/restreamer:latest}"
RESTREAMER_URL="http://localhost:${RESTREAMER_PORT}"
RESTREAMER_USER="${RESTREAMER_USER:-admin}"
RESTREAMER_PASS="${RESTREAMER_PASS:-admin}"
CLEANUP_ON_EXIT=true
KEEP_CONTAINER=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --keep-container)
            KEEP_CONTAINER=true
            shift
            ;;
        --no-cleanup)
            CLEANUP_ON_EXIT=false
            shift
            ;;
        --port)
            RESTREAMER_PORT="$2"
            RESTREAMER_URL="http://localhost:${RESTREAMER_PORT}"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --keep-container  Keep Docker container running after tests"
            echo "  --no-cleanup      Don't cleanup Docker resources"
            echo "  --port PORT       Use custom port (default: 8080)"
            echo "  --help            Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

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

log_section() {
    echo ""
    echo "========================================"
    echo " $1"
    echo "========================================"
    echo ""
}

# Cleanup function
cleanup() {
    if [ "$CLEANUP_ON_EXIT" = true ] && [ "$KEEP_CONTAINER" = false ]; then
        log_section "Cleanup"

        if docker ps -q -f name="${DOCKER_CONTAINER}" | grep -q .; then
            log_info "Stopping Docker container..."
            docker stop "${DOCKER_CONTAINER}" >/dev/null 2>&1 || true
        fi

        if docker ps -aq -f name="${DOCKER_CONTAINER}" | grep -q .; then
            log_info "Removing Docker container..."
            docker rm "${DOCKER_CONTAINER}" >/dev/null 2>&1 || true
        fi

        log_success "Cleanup complete"
    elif [ "$KEEP_CONTAINER" = true ]; then
        log_warning "Container '${DOCKER_CONTAINER}' is still running"
        log_info "Connect to Restreamer at: ${RESTREAMER_URL}"
        log_info "Stop with: docker stop ${DOCKER_CONTAINER}"
    fi
}

# Set trap for cleanup
trap cleanup EXIT INT TERM

# Main execution
main() {
    log_section "OBS Polyemesis - Restreamer Integration Tests"

    # Check Docker
    log_info "Checking Docker..."
    if ! command -v docker >/dev/null 2>&1; then
        log_error "Docker not found. Please install Docker first."
        exit 1
    fi
    log_success "Docker found: $(docker --version | head -1)"

    # Check Python
    log_info "Checking Python..."
    if ! command -v python3 >/dev/null 2>&1; then
        log_error "Python 3 not found. Please install Python 3.8+"
        exit 1
    fi
    log_success "Python found: $(python3 --version)"

    # Check Python dependencies
    log_info "Checking Python dependencies..."
    if ! python3 -c "import requests" 2>/dev/null; then
        log_warning "requests module not found"
        log_info "Installing Python dependencies..."
        pip3 install requests --quiet
    fi
    log_success "Python dependencies ready"

    # Stop existing container
    log_section "Docker Setup"
    if docker ps -q -f name="${DOCKER_CONTAINER}" | grep -q .; then
        log_warning "Existing container found, stopping..."
        docker stop "${DOCKER_CONTAINER}" >/dev/null 2>&1 || true
        docker rm "${DOCKER_CONTAINER}" >/dev/null 2>&1 || true
    fi

    # Pull latest image
    log_info "Pulling Restreamer image: ${RESTREAMER_IMAGE}"
    docker pull "${RESTREAMER_IMAGE}" || {
        log_warning "Failed to pull latest image, using cached version"
    }

    # Start Restreamer
    log_info "Starting Restreamer container..."
    docker run -d \
        --name "${DOCKER_CONTAINER}" \
        -p "${RESTREAMER_PORT}:8080" \
        -p "${RTMP_PORT}:1935" \
        -e "RESTREAMER_USERNAME=${RESTREAMER_USER}" \
        -e "RESTREAMER_PASSWORD=${RESTREAMER_PASS}" \
        "${RESTREAMER_IMAGE}" >/dev/null

    if [ $? -ne 0 ]; then
        log_error "Failed to start Restreamer container"
        exit 1
    fi

    log_success "Restreamer container started"

    # Wait for Restreamer to be ready
    log_info "Waiting for Restreamer to be ready..."
    MAX_WAIT=60
    WAIT_COUNT=0

    while [ $WAIT_COUNT -lt $MAX_WAIT ]; do
        if curl -sf "${RESTREAMER_URL}/api/v3/process" -u "${RESTREAMER_USER}:${RESTREAMER_PASS}" >/dev/null 2>&1; then
            log_success "Restreamer is ready!"
            break
        fi

        echo -n "."
        sleep 2
        WAIT_COUNT=$((WAIT_COUNT + 2))
    done

    if [ $WAIT_COUNT -ge $MAX_WAIT ]; then
        log_error "Restreamer failed to start within ${MAX_WAIT} seconds"
        log_info "Container logs:"
        docker logs "${DOCKER_CONTAINER}" | tail -20
        exit 1
    fi

    echo ""

    # Verify Restreamer API
    log_info "Verifying Restreamer API..."
    ABOUT_RESPONSE=$(curl -sf "${RESTREAMER_URL}/api/v3/process" -u "${RESTREAMER_USER}:${RESTREAMER_PASS}" 2>/dev/null || echo "[]")
    log_success "API responding: $(echo "$ABOUT_RESPONSE" | jq length 2>/dev/null || echo "OK")"

    # Run integration tests
    log_section "Running Integration Tests"

    export RESTREAMER_URL
    export RESTREAMER_USER
    export RESTREAMER_PASS

    cd "$(dirname "$0")"

    log_info "Test suite: Comprehensive Restreamer Integration"
    log_info "Test file: test-restreamer-integration.py"
    echo ""

    python3 test-restreamer-integration.py
    TEST_EXIT_CODE=$?

    echo ""

    # Test results
    if [ $TEST_EXIT_CODE -eq 0 ]; then
        log_section "Test Results"
        log_success "All integration tests passed!"

        if [ "$KEEP_CONTAINER" = true ]; then
            log_info "Restreamer is still running at: ${RESTREAMER_URL}"
            log_info "Username: ${RESTREAMER_USER}"
            log_info "Password: ${RESTREAMER_PASS}"
        fi

        exit 0
    else
        log_section "Test Results"
        log_error "Some integration tests failed"

        log_info "Container logs (last 30 lines):"
        docker logs "${DOCKER_CONTAINER}" | tail -30

        exit $TEST_EXIT_CODE
    fi
}

# Run main
main "$@"
