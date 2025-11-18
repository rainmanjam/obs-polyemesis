#!/bin/bash
# Create macOS installer package for OBS Polyemesis
# Usage: ./scripts/macos-package.sh [options]

set -e

# Configuration
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
VERSION="${VERSION:-}"
OUTPUT_DIR="${OUTPUT_DIR:-$BUILD_DIR/installers}"
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

Create macOS installer package (.pkg) for OBS Polyemesis.

Options:
    -h, --help              Show this help message
    --build-dir DIR         Build directory (default: build)
    --build-type TYPE       Build type (default: Release)
    --version VERSION       Package version (auto-detected if not specified)
    --output-dir DIR        Output directory for package (default: build/installers)
    --build-first           Build the plugin before packaging
    -v, --verbose           Verbose output

Environment Variables:
    BUILD_DIR               Override build directory
    BUILD_TYPE              Override build type
    VERSION                 Package version
    OUTPUT_DIR              Override output directory
    VERBOSE                 Enable verbose output (1 or 0)

Examples:
    # Create package from existing build
    $0

    # Build and package
    $0 --build-first

    # Create package with specific version
    $0 --version 1.0.0

    # Create package in custom location
    $0 --output-dir ~/Desktop

EOF
    exit 0
}

# Parse arguments
BUILD_FIRST=0
while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            usage
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --version)
            VERSION="$2"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --build-first)
            BUILD_FIRST=1
            shift
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
    log_info "macOS Packager for OBS Polyemesis"
    log_debug "Build directory: $BUILD_DIR"
    log_debug "Build type: $BUILD_TYPE"
    log_debug "Output directory: $OUTPUT_DIR"

    # Build if requested
    if [ $BUILD_FIRST -eq 1 ]; then
        log_info "Building plugin..."
        ./scripts/macos-build.sh --build-type "$BUILD_TYPE" --build-dir "$BUILD_DIR"
    fi

    # Check if build directory exists
    if [ ! -d "$BUILD_DIR" ]; then
        log_error "Build directory not found: $BUILD_DIR"
        log_info "Run with --build-first to build the project first"
        exit 1
    fi

    # Detect version if not specified
    if [ -z "$VERSION" ]; then
        if git rev-parse --git-dir > /dev/null 2>&1; then
            # Try to get version from git tag
            VERSION=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
            if [ -z "$VERSION" ]; then
                # Fallback to commit hash
                VERSION=$(git rev-parse --short HEAD)
                log_warn "No version tag found, using commit hash: $VERSION"
            else
                # Remove 'v' prefix if present
                VERSION="${VERSION#v}"
                log_info "Detected version from git tag: $VERSION"
            fi
        else
            VERSION="dev"
            log_warn "Not a git repository, using version: $VERSION"
        fi
    else
        log_info "Using specified version: $VERSION"
    fi

    # Check if packaging script exists
    PACKAGING_SCRIPT="packaging/macos/create-installer.sh"
    if [ ! -f "$PACKAGING_SCRIPT" ]; then
        log_error "Packaging script not found: $PACKAGING_SCRIPT"
        exit 1
    fi

    if [ ! -x "$PACKAGING_SCRIPT" ]; then
        log_debug "Making packaging script executable..."
        chmod +x "$PACKAGING_SCRIPT"
    fi

    # Create output directory
    mkdir -p "$OUTPUT_DIR"

    # Run packaging script
    log_info "Creating installer package..."
    log_debug "Version: $VERSION"
    echo ""

    if "$PACKAGING_SCRIPT" "$VERSION"; then
        log_info "✓ Package created successfully"
    else
        log_error "✗ Package creation failed"
        exit 1
    fi

    # Find and display the created package
    echo ""
    log_info "Package details:"

    mapfile -t PKG_FILES < <(find "$OUTPUT_DIR" -name "*.pkg" -type f 2>/dev/null || true)

    if [ ${#PKG_FILES[@]} -eq 0 ]; then
        # Try alternative location
        mapfile -t PKG_FILES < <(find "$BUILD_DIR" -name "*.pkg" -type f 2>/dev/null || true)
    fi

    if [ ${#PKG_FILES[@]} -gt 0 ]; then
        for pkg in "${PKG_FILES[@]}"; do
            log_info "  Package: $pkg"
            if [ -f "$pkg" ]; then
                PKG_SIZE=$(du -h "$pkg" | cut -f1)
                log_info "  Size: $PKG_SIZE"

                # Show SHA256 if available
                SHA_FILE="${pkg}.sha256"
                if [ -f "$SHA_FILE" ]; then
                    SHA=$(cat "$SHA_FILE" | cut -d' ' -f1)
                    log_info "  SHA256: $SHA"
                else
                    # Generate SHA256
                    SHA=$(shasum -a 256 "$pkg" | cut -d' ' -f1)
                    echo "$SHA  $(basename "$pkg")" > "$SHA_FILE"
                    log_info "  SHA256: $SHA"
                    log_debug "Created SHA256 file: $SHA_FILE"
                fi
            fi
        done

        echo ""
        log_info "To install the package:"
        log_info "  sudo installer -pkg \"${PKG_FILES[0]}\" -target /"
    else
        log_warn "Package file (.pkg) not found in expected locations"
        log_info "Searching for package in build directory..."
        find "$BUILD_DIR" -name "*.pkg" 2>/dev/null || true
    fi

    echo ""
    log_info "Packaging complete!"
}

main
