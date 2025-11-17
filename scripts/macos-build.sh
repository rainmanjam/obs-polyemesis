#!/bin/bash
# Build OBS Polyemesis on macOS
# Usage: ./scripts/macos-build.sh [options]

set -e

# Configuration
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-build}"
ENABLE_QT="${ENABLE_QT:-ON}"
ENABLE_FRONTEND_API="${ENABLE_FRONTEND_API:-ON}"
ENABLE_TESTING="${ENABLE_TESTING:-OFF}"
CMAKE_GENERATOR="${CMAKE_GENERATOR:-Xcode}"
ARCHITECTURES="${MACOS_ARCHITECTURES:-arm64;x86_64}"
CLEAN_BUILD=0
VERBOSE="${VERBOSE:-0}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_debug() {
    if [ "$VERBOSE" = "1" ]; then
        echo -e "${BLUE}[DEBUG]${NC} $1"
    fi
}

# Display usage
usage() {
    cat << EOF
Usage: $0 [options]

Build OBS Polyemesis plugin on macOS.

Options:
    -h, --help              Show this help message
    --clean                 Clean build directory before building
    --build-type TYPE       Build type: Release, Debug, RelWithDebInfo (default: Release)
    --build-dir DIR         Build directory (default: build)
    --no-qt                 Disable Qt UI features
    --no-frontend           Disable OBS frontend API
    --enable-tests          Enable building tests
    --generator GEN         CMake generator: Xcode, Ninja (default: Xcode)
    --arch ARCHS            Architectures to build (default: arm64;x86_64)
                            Use "arm64" for Apple Silicon only
                            Use "x86_64" for Intel only
                            Use "arm64;x86_64" for universal binary
    -v, --verbose           Verbose output

Environment Variables:
    BUILD_TYPE              Override build type
    BUILD_DIR               Override build directory
    ENABLE_QT               Enable/disable Qt (ON/OFF)
    ENABLE_FRONTEND_API     Enable/disable frontend API (ON/OFF)
    ENABLE_TESTING          Enable/disable tests (ON/OFF)
    CMAKE_GENERATOR         CMake generator to use
    MACOS_ARCHITECTURES     Architectures to build
    VERBOSE                 Enable verbose output (1 or 0)

Examples:
    # Standard universal binary build
    $0

    # Clean release build
    $0 --clean

    # Debug build with tests
    $0 --build-type Debug --enable-tests

    # Apple Silicon only build
    $0 --arch arm64

    # Build without Qt
    $0 --no-qt

    # Use Ninja instead of Xcode
    $0 --generator Ninja

EOF
    exit 0
}

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            usage
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --no-qt)
            ENABLE_QT="OFF"
            shift
            ;;
        --no-frontend)
            ENABLE_FRONTEND_API="OFF"
            shift
            ;;
        --enable-tests)
            ENABLE_TESTING="ON"
            shift
            ;;
        --generator)
            CMAKE_GENERATOR="$2"
            shift 2
            ;;
        --arch)
            ARCHITECTURES="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Main execution
main() {
    log_info "macOS Build Script for OBS Polyemesis"
    log_debug "Build type: $BUILD_TYPE"
    log_debug "Build directory: $BUILD_DIR"
    log_debug "Generator: $CMAKE_GENERATOR"
    log_debug "Architectures: $ARCHITECTURES"
    log_debug "Qt: $ENABLE_QT"
    log_debug "Frontend API: $ENABLE_FRONTEND_API"
    log_debug "Testing: $ENABLE_TESTING"

    # Check for required tools
    log_info "Checking required tools..."

    if ! command -v cmake &> /dev/null; then
        log_error "cmake not found. Install with: brew install cmake"
        exit 1
    fi

    if [ "$CMAKE_GENERATOR" = "Ninja" ] && ! command -v ninja &> /dev/null; then
        log_error "ninja not found. Install with: brew install ninja"
        exit 1
    fi

    # Check for OBS Studio
    if [ ! -d "/Applications/OBS.app" ]; then
        log_warn "OBS Studio not found at /Applications/OBS.app"
        log_warn "Install with: brew install --cask obs"
        log_warn "Continuing anyway, but build may fail..."
    else
        log_info "✓ Found OBS Studio"
    fi

    # Check for dependencies
    log_info "Checking dependencies..."
    local missing_deps=()

    if ! pkg-config --exists jansson 2>/dev/null; then
        missing_deps+=("jansson")
    fi

    if ! pkg-config --exists libcurl 2>/dev/null; then
        missing_deps+=("curl")
    fi

    if [ "$ENABLE_QT" = "ON" ] && ! command -v qmake &> /dev/null && [ ! -d "/Applications/OBS.app/Contents/Frameworks/QtCore.framework" ]; then
        missing_deps+=("qt6")
    fi

    if [ ${#missing_deps[@]} -gt 0 ]; then
        log_warn "Missing dependencies: ${missing_deps[*]}"
        log_info "Install with: brew install ${missing_deps[*]}"
        read -p "Continue anyway? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    else
        log_info "✓ All dependencies found"
    fi

    # Clean build if requested
    if [ $CLEAN_BUILD -eq 1 ]; then
        log_info "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
        log_info "✓ Build directory cleaned"
    fi

    # Create build directory
    mkdir -p "$BUILD_DIR"

    # Configure
    log_info "Configuring build..."
    log_debug "Running cmake configuration..."

    CMAKE_ARGS=(
        -B "$BUILD_DIR"
        -G "$CMAKE_GENERATOR"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DENABLE_FRONTEND_API="$ENABLE_FRONTEND_API"
        -DENABLE_QT="$ENABLE_QT"
        -DENABLE_TESTING="$ENABLE_TESTING"
        -DCMAKE_OSX_ARCHITECTURES="$ARCHITECTURES"
    )

    if [ "$VERBOSE" = "1" ]; then
        CMAKE_ARGS+=(--debug-output)
    fi

    log_debug "CMake command: cmake ${CMAKE_ARGS[*]}"

    if cmake "${CMAKE_ARGS[@]}"; then
        log_info "✓ Configuration successful"
    else
        log_error "Configuration failed"
        exit 1
    fi

    # Build
    log_info "Building..."

    BUILD_ARGS=(
        --build "$BUILD_DIR"
        --config "$BUILD_TYPE"
    )

    if [ "$VERBOSE" = "1" ]; then
        BUILD_ARGS+=(--verbose)
    fi

    # Add parallel jobs for faster builds
    if [ "$CMAKE_GENERATOR" = "Ninja" ]; then
        BUILD_ARGS+=(--parallel "$(sysctl -n hw.ncpu)")
    elif [ "$CMAKE_GENERATOR" = "Xcode" ]; then
        BUILD_ARGS+=(-- -jobs "$(sysctl -n hw.ncpu)")
    fi

    log_debug "Build command: cmake ${BUILD_ARGS[*]}"

    if cmake "${BUILD_ARGS[@]}"; then
        log_info "✓ Build successful"
    else
        log_error "Build failed"
        exit 1
    fi

    # Display build info
    echo ""
    log_info "Build completed successfully!"
    log_info ""
    log_info "Build directory: $BUILD_DIR"
    log_info "Build type: $BUILD_TYPE"
    log_info "Architectures: $ARCHITECTURES"

    # Find the built plugin
    if [ "$CMAKE_GENERATOR" = "Xcode" ]; then
        PLUGIN_PATH="$BUILD_DIR/$BUILD_TYPE/obs-polyemesis.so"
    else
        PLUGIN_PATH="$BUILD_DIR/obs-polyemesis.so"
    fi

    if [ -f "$PLUGIN_PATH" ]; then
        log_info "Plugin: $PLUGIN_PATH"

        # Show plugin info
        log_info ""
        log_info "Plugin details:"
        file "$PLUGIN_PATH" | sed 's/^/  /'

        # Show dependencies
        if command -v otool &> /dev/null; then
            log_info ""
            log_info "Dependencies:"
            otool -L "$PLUGIN_PATH" | tail -n +2 | sed 's/^/  /'
        fi
    else
        log_warn "Plugin not found at expected location: $PLUGIN_PATH"
    fi

    echo ""
    log_info "To install the plugin, run:"
    log_info "  ./scripts/macos-install.sh"
    log_info ""
    log_info "To create a package, run:"
    log_info "  ./scripts/macos-package.sh"
}

main
