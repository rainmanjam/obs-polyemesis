#!/bin/bash
set -e

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Cleanup on exit
cleanup() {
    log_info "Cleaning up Docker Compose services..."
    cd "$PROJECT_ROOT"
    docker-compose -f docker-compose.integration.yml down -v
}
trap cleanup EXIT

log_info "Starting integration test environment..."

# Start services
cd "$PROJECT_ROOT"
docker-compose -f docker-compose.integration.yml up --build --abort-on-container-exit

# Check exit code
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    log_success "Integration tests passed!"
else
    echo "Integration tests failed with exit code $EXIT_CODE"
    exit $EXIT_CODE
fi
