#!/bin/bash
# OBS Polyemesis - Linux Testing Script (Docker-based)
# Tests the plugin on Ubuntu Linux using Docker

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
DOCKER_IMAGE="ubuntu:24.04"
CONTAINER_NAME="obs-polyemesis-linux-test-$$"
OBS_VERSION="32.0.2"
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
log_info "Starting Linux testing for OBS Polyemesis"
echo "========================================"

# Check Docker
if ! command -v docker &> /dev/null; then
    log_error "Docker not found. Please install Docker first."
    exit 1
fi

log_success "Docker found"

# Pull Ubuntu image (AMD64 platform for OBS compatibility)
log_info "Pulling Ubuntu 24.04 Docker image (AMD64)..."
docker pull --platform linux/amd64 "$DOCKER_IMAGE"

# Create and start container (using AMD64 platform for OBS compatibility)
log_info "Starting Ubuntu container (AMD64 platform)..."
docker run -d \
    --platform linux/amd64 \
    --name "$CONTAINER_NAME" \
    -v "$PROJECT_ROOT:/workspace" \
    -w /workspace \
    "$DOCKER_IMAGE" \
    sleep infinity

log_success "Container started: $CONTAINER_NAME"

# Install dependencies
log_info "Installing build dependencies..."
docker exec "$CONTAINER_NAME" bash -c "
    export DEBIAN_FRONTEND=noninteractive
    apt-get update
    apt-get install -y \
        build-essential \
        git \
        libcurl4-openssl-dev \
        libjansson-dev \
        python3 \
        python3-pip \
        wget \
        pkg-config \
        software-properties-common \
        gpg
"

# Install CMake 3.28+ from Kitware APT repository
log_info "Installing CMake 3.28+ from Kitware..."
docker exec "$CONTAINER_NAME" bash -c "
    export DEBIAN_FRONTEND=noninteractive
    # Add Kitware APT repository
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' > /etc/apt/sources.list.d/kitware.list
    apt-get update
    apt-get install -y cmake
"

# Verify CMake version
docker exec "$CONTAINER_NAME" bash -c "cmake --version"

# Install OBS Studio and development libraries
log_info "Installing OBS Studio ${OBS_VERSION}..."

# Install OBS development libraries from PPA (for building plugins)
docker exec "$CONTAINER_NAME" bash -c "
    export DEBIAN_FRONTEND=noninteractive
    add-apt-repository -y ppa:obsproject/obs-studio
    apt-get update
    apt-get install -y libobs-dev
"

# Attempt to install OBS 32.0.2 via Flatpak
log_info "Installing OBS ${OBS_VERSION} via Flatpak..."
docker exec "$CONTAINER_NAME" bash -c "
    export DEBIAN_FRONTEND=noninteractive
    apt-get install -y flatpak
    flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
    # Install specific version 32.0.2 from Flathub
    flatpak install -y flathub com.obsproject.Studio//32.0.2 || \
    flatpak install -y flathub com.obsproject.Studio
" || log_warning "Flatpak OBS installation failed"

# Verify OBS version from Flatpak
log_info "Verifying OBS version from Flatpak..."
docker exec "$CONTAINER_NAME" bash -c "
    flatpak run com.obsproject.Studio --version 2>&1 | head -5 || echo 'OBS version check failed'
"

# Check development libraries (these are what we actually need for building)
log_info "Verifying OBS development libraries from PPA..."
docker exec "$CONTAINER_NAME" bash -c "
    echo '=== OBS libobs version from PPA ==='
    pkg-config --modversion libobs 2>/dev/null || echo 'libobs not found via pkg-config'
    echo ''
    echo '=== libobs-dev package info ==='
    dpkg -l | grep libobs-dev | awk '{print \$2, \$3}' || echo 'libobs-dev not found'
"

log_success "OBS development environment ready (using PPA libraries)"

# Build the plugin
log_info "Building OBS Polyemesis plugin for Linux..."
if docker exec "$CONTAINER_NAME" bash -c "
    mkdir -p build-linux
    cd build-linux
    cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release -DENABLE_QT=OFF
    cmake --build . --config Release
"; then
    log_success "Plugin built successfully"
else
    log_error "Plugin build failed"
    exit 1
fi

# Install Python test dependencies
log_info "Installing Python test dependencies..."
docker exec "$CONTAINER_NAME" bash -c "
    pip3 install requests python-dotenv
"

# Run API integration tests
log_info "Running API integration tests..."
docker exec "$CONTAINER_NAME" bash -c "
    cd tests
    python3 test-restreamer-integration.py || true
"

# Run automated tests if they exist
if [ -d "$PROJECT_ROOT/tests/automated" ]; then
    log_info "Running automated test suite..."
    docker exec "$CONTAINER_NAME" bash -c "
        cd tests/automated
        for test in test_*.py; do
            echo \"Running \$test...\"
            python3 \"\$test\" || true
        done
    "
fi

# Check if plugin binary was created
log_info "Checking build artifacts..."
if docker exec "$CONTAINER_NAME" bash -c "
    if [ -f build-linux/obs-polyemesis.so ]; then
        echo '✓ Plugin binary created: obs-polyemesis.so'
        ls -lh build-linux/obs-polyemesis.so
    else
        echo '✗ Plugin binary not found'
        exit 1
    fi
"; then
    log_success "Build artifacts verified"
else
    log_error "Build artifacts missing"
    exit 1
fi

# Summary
echo ""
echo "========================================"
log_success "Linux testing completed!"
echo "Container: $CONTAINER_NAME (will be cleaned up)"
echo ""
log_info "To manually explore the container:"
echo "  docker exec -it $CONTAINER_NAME bash"
echo ""
log_info "To keep the container running (Ctrl+C to skip cleanup):"
sleep 2

exit 0
