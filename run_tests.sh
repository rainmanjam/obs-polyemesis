#!/bin/bash
# Quick start script for running API integration tests locally
# This script handles the full test lifecycle:
# 1. Start Docker test environment
# 2. Wait for services to be ready
# 3. Run API integration tests
# 4. Cleanup

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

cleanup() {
    log_info "Cleaning up test environment..."
    docker-compose -f docker-compose.test.yml down -v 2>/dev/null || true
}

# Trap EXIT to ensure cleanup
trap cleanup EXIT INT TERM

echo "========================================="
echo "OBS Polyemesis - API Integration Tests"
echo "========================================="
echo ""

# Check prerequisites
log_info "Checking prerequisites..."

if ! command -v docker &> /dev/null; then
    log_error "Docker is not installed"
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    log_error "Docker Compose is not installed"
    exit 1
fi

if ! command -v jq &> /dev/null; then
    log_warn "jq is not installed (recommended for debugging)"
fi

# Cleanup any previous test environment
cleanup

# Start test environment
log_info "Starting Restreamer test environment..."
docker-compose -f docker-compose.test.yml up -d

# Wait for services to be healthy
log_info "Waiting for services to be ready (this may take 30-60 seconds)..."
sleep 10

# Wait for Restreamer API
RETRIES=30
RETRY_COUNT=0
while [ $RETRY_COUNT -lt $RETRIES ]; do
    if curl -sf http://localhost:8080/api/v3/about > /dev/null 2>&1; then
        log_info "Restreamer is ready!"
        break
    fi
    echo -n "."
    sleep 2
    RETRY_COUNT=$((RETRY_COUNT + 1))
done

if [ $RETRY_COUNT -eq $RETRIES ]; then
    log_error "Restreamer failed to start within 60 seconds"
    log_info "Showing container logs:"
    docker-compose -f docker-compose.test.yml logs --tail=50
    exit 1
fi

echo ""

# Show container status
log_info "Container status:"
docker-compose -f docker-compose.test.yml ps

echo ""

# Run tests
log_info "Running API integration tests..."
echo ""

chmod +x tests/api_integration_test.sh
if ./tests/api_integration_test.sh; then
    echo ""
    log_info "Tests completed successfully!"
    exit 0
else
    echo ""
    log_error "Tests failed!"
    echo ""
    log_info "Showing Restreamer logs:"
    docker-compose -f docker-compose.test.yml logs --tail=100 restreamer
    exit 1
fi
