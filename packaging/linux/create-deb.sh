#!/bin/bash
##############################################################################
# OBS Polyemesis - Debian Package Creation Script
##############################################################################
#
# This script creates a .deb package for Ubuntu/Debian that installs the
# plugin into the OBS Studio plugins directory.
#
# Requirements:
#   - dpkg-deb
#   - Built plugin binary in build/Release
#
# Usage:
#   ./create-deb.sh [version] [architecture]
#
# Example:
#   ./create-deb.sh 1.0.0 amd64
#
##############################################################################

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
VERSION="${1:-1.0.0}"
ARCH="${2:-amd64}"
PLUGIN_NAME="obs-polyemesis"

# Paths
BUILD_DIR="${PROJECT_ROOT}/build"
PACKAGE_ROOT="${BUILD_DIR}/deb-package"
OUTPUT_DIR="${BUILD_DIR}/installers"

# Plugin binary path
PLUGIN_SO="${BUILD_DIR}/lib${PLUGIN_NAME}.so"
if [[ ! -f "${PLUGIN_SO}" ]]; then
    PLUGIN_SO="${BUILD_DIR}/${PLUGIN_NAME}.so"
fi

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Cleanup function
cleanup() {
    log_info "Cleaning up temporary files..."
    rm -rf "${PACKAGE_ROOT}"
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    if [[ ! -f "${PLUGIN_SO}" ]]; then
        log_error "Plugin binary not found at: ${PLUGIN_SO}"
        log_error "Please build the plugin first:"
        log_error "  cmake -B build -DCMAKE_BUILD_TYPE=Release"
        log_error "  cmake --build build"
        exit 1
    fi

    if ! command -v dpkg-deb &> /dev/null; then
        log_error "dpkg-deb not found. Please install dpkg tools."
        exit 1
    fi

    log_info "All prerequisites satisfied"
}

# Prepare package structure
prepare_package_structure() {
    log_info "Preparing package structure..."

    # Clean up old package root
    rm -rf "${PACKAGE_ROOT}"

    # Create directory structure
    mkdir -p "${PACKAGE_ROOT}/DEBIAN"
    mkdir -p "${PACKAGE_ROOT}/usr/lib/obs-plugins"
    mkdir -p "${PACKAGE_ROOT}/usr/share/obs/obs-plugins/${PLUGIN_NAME}/locale"
    mkdir -p "${PACKAGE_ROOT}/usr/share/doc/${PLUGIN_NAME}"

    # Copy plugin binary
    log_info "Copying plugin binary..."
    cp "${PLUGIN_SO}" "${PACKAGE_ROOT}/usr/lib/obs-plugins/"

    # Copy locale files if they exist
    if [[ -d "${PROJECT_ROOT}/data/locale" ]]; then
        log_info "Copying locale files..."
        cp -r "${PROJECT_ROOT}/data/locale/"* "${PACKAGE_ROOT}/usr/share/obs/obs-plugins/${PLUGIN_NAME}/locale/" 2>/dev/null || true
    fi

    # Copy documentation
    if [[ -f "${PROJECT_ROOT}/README.md" ]]; then
        cp "${PROJECT_ROOT}/README.md" "${PACKAGE_ROOT}/usr/share/doc/${PLUGIN_NAME}/"
    fi
    if [[ -f "${PROJECT_ROOT}/LICENSE" ]]; then
        cp "${PROJECT_ROOT}/LICENSE" "${PACKAGE_ROOT}/usr/share/doc/${PLUGIN_NAME}/copyright"
    fi

    # Set proper permissions
    chmod -R 755 "${PACKAGE_ROOT}/usr/lib/obs-plugins"
    chmod -R 644 "${PACKAGE_ROOT}/usr/share/obs/obs-plugins/${PLUGIN_NAME}/locale/"* 2>/dev/null || true
    chmod -R 644 "${PACKAGE_ROOT}/usr/share/doc/${PLUGIN_NAME}/"*

    log_info "Package structure prepared"
}

# Create control file
create_control_file() {
    log_info "Creating control file..."

    cat > "${PACKAGE_ROOT}/DEBIAN/control" <<CONTROL
Package: ${PLUGIN_NAME}
Version: ${VERSION}
Section: video
Priority: optional
Architecture: ${ARCH}
Depends: obs-studio (>= 28.0), libcurl4, libjansson4
Maintainer: rainmanjam <noreply@github.com>
Homepage: https://github.com/rainmanjam/${PLUGIN_NAME}
Description: datarhei Restreamer control plugin for OBS Studio
 OBS Polyemesis is a comprehensive plugin for controlling and monitoring
 datarhei Restreamer with advanced multistreaming capabilities including
 orientation-aware routing.
 .
 Features:
  - Complete Restreamer control from within OBS
  - Multiple plugin types: Source, Output, and Dock UI
  - Advanced multistreaming to multiple platforms
  - Orientation-aware routing (horizontal/vertical)
  - Real-time monitoring of CPU, memory, uptime
CONTROL

    chmod 644 "${PACKAGE_ROOT}/DEBIAN/control"
    log_info "Control file created"
}

# Create postinst script
create_postinst() {
    log_info "Creating postinst script..."

    cat > "${PACKAGE_ROOT}/DEBIAN/postinst" <<'POSTINST'
#!/bin/bash
set -e

echo "OBS Polyemesis plugin installed successfully!"
echo ""
echo "To use the plugin:"
echo "1. Launch OBS Studio"
echo "2. Go to View → Docks → Restreamer Control"
echo "3. Configure your restreamer connection"
echo ""
echo "For documentation, visit:"
echo "https://github.com/rainmanjam/obs-polyemesis"

exit 0
POSTINST

    chmod 755 "${PACKAGE_ROOT}/DEBIAN/postinst"
    log_info "Postinst script created"
}

# Build package
build_package() {
    log_info "Building .deb package..."

    mkdir -p "${OUTPUT_DIR}"
    local DEB_FILE="${OUTPUT_DIR}/${PLUGIN_NAME}_${VERSION}_${ARCH}.deb"

    dpkg-deb --build --root-owner-group "${PACKAGE_ROOT}" "${DEB_FILE}"

    if [[ -f "${DEB_FILE}" ]]; then
        log_info "Package created: ${DEB_FILE}"

        # Get package size
        local PKG_SIZE=$(du -h "${DEB_FILE}" | cut -f1)
        log_info "Package size: ${PKG_SIZE}"

        # Verify package
        log_info "Verifying package..."
        dpkg-deb --info "${DEB_FILE}"

        # Calculate SHA256
        log_info "Calculating SHA256 checksum..."
        local SHA256=$(sha256sum "${DEB_FILE}" | cut -d' ' -f1)
        echo "${SHA256}" > "${DEB_FILE}.sha256"
        log_info "SHA256: ${SHA256}"

        return 0
    else
        log_error "Failed to create package"
        return 1
    fi
}

# Main execution
main() {
    log_info "====================================="
    log_info "OBS Polyemesis Debian Package Build"
    log_info "Version: ${VERSION}"
    log_info "Architecture: ${ARCH}"
    log_info "====================================="

    check_prerequisites
    prepare_package_structure
    create_control_file
    create_postinst

    if build_package; then
        cleanup
        log_info "====================================="
        log_info "✓ Package created successfully!"
        log_info "====================================="
        log_info ""
        log_info "Package location:"
        log_info "  ${OUTPUT_DIR}/${PLUGIN_NAME}_${VERSION}_${ARCH}.deb"
        log_info ""
        log_info "To install:"
        log_info "  sudo dpkg -i ${OUTPUT_DIR}/${PLUGIN_NAME}_${VERSION}_${ARCH}.deb"
        log_info "  sudo apt-get install -f  # Fix dependencies if needed"
        log_info ""
        log_info "To test:"
        log_info "  dpkg -L ${PLUGIN_NAME}  # List installed files"
        log_info "  dpkg -s ${PLUGIN_NAME}  # Show package status"
        return 0
    else
        cleanup
        log_error "====================================="
        log_error "✗ Package creation failed"
        log_error "====================================="
        return 1
    fi
}

# Run main function
main "$@"
