#!/bin/bash
##############################################################################
# OBS Polyemesis - macOS Installer Creation Script
##############################################################################
#
# This script creates a .pkg installer for macOS that installs the plugin
# into the user's OBS Studio plugins directory.
#
# Requirements:
#   - Xcode Command Line Tools
#   - Built plugin binary in build/Debug or build/Release
#
# Usage:
#   ./create-installer.sh [version]
#
# Example:
#   ./create-installer.sh 1.0.0
#
##############################################################################

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
VERSION="${1:-1.0.0}"
PLUGIN_NAME="obs-polyemesis"
IDENTIFIER="com.github.rainmanjam.obs-polyemesis"

# Paths
BUILD_DIR="${PROJECT_ROOT}/build"
PACKAGE_ROOT="${BUILD_DIR}/package-root"
SCRIPTS_DIR="${BUILD_DIR}/package-scripts"
OUTPUT_DIR="${BUILD_DIR}/installers"

# Plugin paths
PLUGIN_BUNDLE="${BUILD_DIR}/Debug/${PLUGIN_NAME}.plugin"
if [[ ! -d "${PLUGIN_BUNDLE}" ]]; then
    PLUGIN_BUNDLE="${BUILD_DIR}/Release/${PLUGIN_NAME}.plugin"
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Cleanup function
cleanup() {
    log_info "Cleaning up temporary files..."
    rm -rf "${PACKAGE_ROOT}" "${SCRIPTS_DIR}"
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    # Check if plugin bundle exists
    if [[ ! -d "${PLUGIN_BUNDLE}" ]]; then
        log_error "Plugin bundle not found at: ${PLUGIN_BUNDLE}"
        log_error "Please build the plugin first:"
        log_error "  cmake -B build -DCMAKE_BUILD_TYPE=Release"
        log_error "  cmake --build build --config Release"
        exit 1
    fi

    # Check if pkgbuild is available
    if ! command -v pkgbuild &> /dev/null; then
        log_error "pkgbuild command not found. Please install Xcode Command Line Tools."
        exit 1
    fi

    # Check if productbuild is available
    if ! command -v productbuild &> /dev/null; then
        log_error "productbuild command not found. Please install Xcode Command Line Tools."
        exit 1
    fi

    log_info "All prerequisites satisfied"
}

# Prepare package structure
prepare_package_structure() {
    log_info "Preparing package structure..."

    # Clean up old package root
    rm -rf "${PACKAGE_ROOT}"
    rm -rf "${SCRIPTS_DIR}"

    # Create package directory structure
    # OBS plugins go to ~/Library/Application Support/obs-studio/plugins/
    mkdir -p "${PACKAGE_ROOT}/Library/Application Support/obs-studio/plugins"

    # Copy plugin bundle
    log_info "Copying plugin bundle..."
    cp -R "${PLUGIN_BUNDLE}" "${PACKAGE_ROOT}/Library/Application Support/obs-studio/plugins/"

    # Set proper permissions
    chmod -R 755 "${PACKAGE_ROOT}/Library/Application Support/obs-studio/plugins/${PLUGIN_NAME}.plugin"

    log_info "Package structure prepared"
}

# Create installation scripts
create_install_scripts() {
    log_info "Creating installation scripts..."

    mkdir -p "${SCRIPTS_DIR}"

    # Preinstall script - Check if OBS is installed
    cat > "${SCRIPTS_DIR}/preinstall" <<'EOF'
#!/bin/bash
# Preinstall script for OBS Polyemesis

# Check if OBS Studio is installed
OBS_APP="/Applications/OBS.app"
if [[ ! -d "${OBS_APP}" ]]; then
    echo "WARNING: OBS Studio not found at ${OBS_APP}"
    echo "Please install OBS Studio before installing this plugin."
    echo "Download from: https://obsproject.com/"
    # Don't fail the installation, just warn
fi

exit 0
EOF

    # Postinstall script - Show success message
    cat > "${SCRIPTS_DIR}/postinstall" <<'EOF'
#!/bin/bash
# Postinstall script for OBS Polyemesis

PLUGIN_DIR="$HOME/Library/Application Support/obs-studio/plugins/obs-polyemesis.plugin"

if [[ -d "${PLUGIN_DIR}" ]]; then
    echo "OBS Polyemesis plugin installed successfully!"
    echo "Plugin location: ${PLUGIN_DIR}"
    echo ""
    echo "To use the plugin:"
    echo "1. Launch OBS Studio"
    echo "2. Go to View → Docks → Restreamer Control"
    echo "3. Configure your restreamer connection"
    echo ""
    echo "For help and documentation, visit:"
    echo "https://github.com/rainmanjam/obs-polyemesis"
else
    echo "ERROR: Plugin installation may have failed."
    echo "Expected location: ${PLUGIN_DIR}"
fi

exit 0
EOF

    chmod +x "${SCRIPTS_DIR}/preinstall"
    chmod +x "${SCRIPTS_DIR}/postinstall"

    log_info "Installation scripts created"
}

# Build component package
build_component_package() {
    log_info "Building component package..."

    local COMPONENT_PKG="${BUILD_DIR}/${PLUGIN_NAME}-component.pkg"

    pkgbuild \
        --root "${PACKAGE_ROOT}" \
        --identifier "${IDENTIFIER}" \
        --version "${VERSION}" \
        --scripts "${SCRIPTS_DIR}" \
        --install-location "/" \
        "${COMPONENT_PKG}"

    if [[ -f "${COMPONENT_PKG}" ]]; then
        log_info "Component package created: ${COMPONENT_PKG}"
    else
        log_error "Failed to create component package"
        exit 1
    fi

    echo "${COMPONENT_PKG}"
}

# Build product package
build_product_package() {
    local COMPONENT_PKG="$1"
    log_info "Building product package..."

    mkdir -p "${OUTPUT_DIR}"
    local PRODUCT_PKG="${OUTPUT_DIR}/${PLUGIN_NAME}-${VERSION}-macos-universal.pkg"

    # Create distribution XML
    local DISTRIBUTION_XML="${BUILD_DIR}/distribution.xml"
    cat > "${DISTRIBUTION_XML}" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>OBS Polyemesis ${VERSION}</title>
    <organization>com.github.rainmanjam</organization>
    <domains enable_localSystem="false"/>
    <options customize="never" require-scripts="true" hostArchitectures="arm64,x86_64"/>

    <!-- Define documents displayed at various steps -->
    <welcome file="welcome.html" mime-type="text/html" />
    <conclusion file="conclusion.html" mime-type="text/html" />
    <readme file="readme.html" mime-type="text/html" />

    <!-- List all component packages -->
    <pkg-ref id="${IDENTIFIER}"/>

    <!-- Define the order for the components listed -->
    <choices-outline>
        <line choice="default">
            <line choice="${IDENTIFIER}"/>
        </line>
    </choices-outline>

    <!-- Define the choice attributes -->
    <choice id="default"/>
    <choice id="${IDENTIFIER}" visible="false">
        <pkg-ref id="${IDENTIFIER}"/>
    </choice>

    <!-- Define package references -->
    <pkg-ref id="${IDENTIFIER}" version="${VERSION}" onConclusion="none">
        $(basename "${COMPONENT_PKG}")
    </pkg-ref>
</installer-gui-script>
EOF

    # Create welcome HTML
    cat > "${BUILD_DIR}/welcome.html" <<EOF
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; padding: 20px; }
        h1 { color: #333; }
        .info { background-color: #f0f0f0; padding: 15px; border-radius: 5px; margin: 15px 0; }
    </style>
</head>
<body>
    <h1>Welcome to OBS Polyemesis ${VERSION}</h1>
    <p>This installer will install the OBS Polyemesis plugin for OBS Studio.</p>

    <div class="info">
        <strong>Prerequisites:</strong>
        <ul>
            <li>OBS Studio 28.0 or later must be installed</li>
            <li>datarhei Restreamer instance (local or remote)</li>
        </ul>
    </div>

    <p><strong>What is OBS Polyemesis?</strong></p>
    <p>OBS Polyemesis is a comprehensive plugin for controlling and monitoring datarhei Restreamer with advanced multistreaming capabilities including orientation-aware routing.</p>

    <p>Click Continue to proceed with the installation.</p>
</body>
</html>
EOF

    # Create conclusion HTML
    cat > "${BUILD_DIR}/conclusion.html" <<EOF
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; padding: 20px; }
        h1 { color: #333; }
        .success { background-color: #d4edda; padding: 15px; border-radius: 5px; margin: 15px 0; border-left: 4px solid #28a745; }
        .next-steps { background-color: #f8f9fa; padding: 15px; border-radius: 5px; margin: 15px 0; }
    </style>
</head>
<body>
    <h1>Installation Complete!</h1>

    <div class="success">
        <strong>✓ OBS Polyemesis ${VERSION} has been successfully installed!</strong>
    </div>

    <div class="next-steps">
        <strong>Next Steps:</strong>
        <ol>
            <li>Launch OBS Studio</li>
            <li>Go to <strong>View → Docks → Restreamer Control</strong></li>
            <li>Configure your restreamer connection (host, port)</li>
            <li>Click "Test Connection" to verify</li>
            <li>Start controlling your restreamer processes!</li>
        </ol>
    </div>

    <p>For documentation and support:</p>
    <ul>
        <li>GitHub: <a href="https://github.com/rainmanjam/obs-polyemesis">github.com/rainmanjam/obs-polyemesis</a></li>
        <li>User Guide: See README.md and USER_GUIDE.md in the repository</li>
    </ul>
</body>
</html>
EOF

    # Create readme HTML
    cat > "${BUILD_DIR}/readme.html" <<EOF
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; padding: 20px; }
        h2 { color: #333; }
        code { background-color: #f0f0f0; padding: 2px 6px; border-radius: 3px; }
    </style>
</head>
<body>
    <h2>Installation Location</h2>
    <p>The plugin will be installed to:</p>
    <code>~/Library/Application Support/obs-studio/plugins/obs-polyemesis.plugin</code>

    <h2>Requirements</h2>
    <ul>
        <li>macOS 11.0 or later (Apple Silicon or Intel)</li>
        <li>OBS Studio 28.0 or later</li>
        <li>datarhei Restreamer instance</li>
    </ul>

    <h2>Features</h2>
    <ul>
        <li>Complete Restreamer control from within OBS</li>
        <li>Multiple plugin types: Source, Output, and Dock UI</li>
        <li>Advanced multistreaming to multiple platforms simultaneously</li>
        <li>Orientation-aware routing (horizontal/vertical)</li>
        <li>Real-time monitoring of CPU, memory, uptime</li>
    </ul>
</body>
</html>
EOF

    # Build the product package
    productbuild \
        --distribution "${DISTRIBUTION_XML}" \
        --package-path "${BUILD_DIR}" \
        --resources "${BUILD_DIR}" \
        "${PRODUCT_PKG}"

    if [[ -f "${PRODUCT_PKG}" ]]; then
        log_info "Product package created: ${PRODUCT_PKG}"

        # Get package size
        local PKG_SIZE=$(du -h "${PRODUCT_PKG}" | cut -f1)
        log_info "Package size: ${PKG_SIZE}"

        # Calculate SHA256
        log_info "Calculating SHA256 checksum..."
        local SHA256=$(shasum -a 256 "${PRODUCT_PKG}" | cut -d' ' -f1)
        echo "${SHA256}" > "${PRODUCT_PKG}.sha256"
        log_info "SHA256: ${SHA256}"

        return 0
    else
        log_error "Failed to create product package"
        return 1
    fi
}

# Main execution
main() {
    log_info "====================================="
    log_info "OBS Polyemesis macOS Installer Build"
    log_info "Version: ${VERSION}"
    log_info "====================================="

    check_prerequisites
    prepare_package_structure
    create_install_scripts

    local component_pkg=$(build_component_package)

    if build_product_package "${component_pkg}"; then
        cleanup
        log_info "====================================="
        log_info "✓ Installer created successfully!"
        log_info "====================================="
        log_info ""
        log_info "Installer location:"
        log_info "  ${OUTPUT_DIR}/${PLUGIN_NAME}-${VERSION}-macos-universal.pkg"
        log_info ""
        log_info "To install:"
        log_info "  open ${OUTPUT_DIR}/${PLUGIN_NAME}-${VERSION}-macos-universal.pkg"
        log_info ""
        log_info "To test:"
        log_info "  1. Double-click the .pkg file"
        log_info "  2. Follow installation prompts"
        log_info "  3. Launch OBS Studio"
        log_info "  4. Check View → Docks → Restreamer Control"
        return 0
    else
        cleanup
        log_error "====================================="
        log_error "✗ Installer creation failed"
        log_error "====================================="
        return 1
    fi
}

# Run main function
main "$@"
