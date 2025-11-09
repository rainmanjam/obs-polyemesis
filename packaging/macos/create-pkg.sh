#!/bin/bash
# macOS .pkg Installer Builder for OBS Polyemesis
# Creates a signed macOS installer package

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Configuration
PLUGIN_NAME="obs-polyemesis"
VERSION="${VERSION:-1.0.0}"
IDENTIFIER="com.rainmanjam.obs-polyemesis"
# User-specific installation (OBS searches ~/Library, not /Library)
INSTALL_LOCATION="Library/Application Support/obs-studio/plugins"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build}"
PKG_DIR="$SCRIPT_DIR/build"
PLUGIN_BUNDLE="${BUILD_DIR}/obs-polyemesis.plugin"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    if ! command -v pkgbuild &> /dev/null; then
        log_error "pkgbuild not found. This tool is required for creating macOS packages."
        exit 1
    fi

    if ! command -v productbuild &> /dev/null; then
        log_error "productbuild not found. This tool is required for creating macOS packages."
        exit 1
    fi

    log_info "Prerequisites check passed"
}

# Verify plugin bundle exists
verify_plugin() {
    log_info "Verifying plugin bundle..."

    if [ ! -d "$PLUGIN_BUNDLE" ]; then
        log_error "Plugin bundle not found at: $PLUGIN_BUNDLE"
        log_error "Please build the plugin first with: cmake --build build --config Release"
        exit 1
    fi

    log_info "Plugin bundle verified: $PLUGIN_BUNDLE"

    # Check binary architecture
    PLUGIN_BINARY="$PLUGIN_BUNDLE/Contents/MacOS/obs-polyemesis"
    if [ -f "$PLUGIN_BINARY" ]; then
        log_info "Plugin binary architectures:"
        lipo -info "$PLUGIN_BINARY" || file "$PLUGIN_BINARY"
    fi
}

# Create package structure
create_package_structure() {
    log_info "Creating package structure..."

    # Clean and create directories
    rm -rf "$PKG_DIR"
    mkdir -p "$PKG_DIR/root/$INSTALL_LOCATION"
    mkdir -p "$PKG_DIR/scripts"
    mkdir -p "$PKG_DIR/resources"

    # Copy plugin bundle (preserving all attributes and architectures)
    cp -RPp "$PLUGIN_BUNDLE" "$PKG_DIR/root/$INSTALL_LOCATION/"

    # Verify architecture after copy
    COPIED_BINARY="$PKG_DIR/root/$INSTALL_LOCATION/obs-polyemesis.plugin/Contents/MacOS/obs-polyemesis"
    if [ -f "$COPIED_BINARY" ]; then
        log_info "Copied binary architectures:"
        lipo -info "$COPIED_BINARY" || file "$COPIED_BINARY"
    fi

    log_info "Package structure created"
}

# Create preinstall script
create_preinstall_script() {
    log_info "Creating preinstall script..."

    cat > "$PKG_DIR/scripts/preinstall" << 'EOF'
#!/bin/bash
# Pre-installation script for OBS Polyemesis

# Check if OBS Studio is installed
if [ ! -d "/Applications/OBS.app" ]; then
    echo "WARNING: OBS Studio does not appear to be installed."
    echo "Please install OBS Studio before installing this plugin."
    echo "Download from: https://obsproject.com"
    # Don't fail - let user install anyway
fi

# Check OBS version (if OBS is installed)
if [ -d "/Applications/OBS.app" ]; then
    OBS_VERSION=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "/Applications/OBS.app/Contents/Info.plist" 2>/dev/null || echo "unknown")
    echo "OBS Studio version: $OBS_VERSION"

    # Extract major version
    MAJOR_VERSION=$(echo "$OBS_VERSION" | cut -d. -f1)

    if [ "$MAJOR_VERSION" -lt 28 ] 2>/dev/null; then
        echo "WARNING: OBS Studio 28.0 or later is required."
        echo "Current version: $OBS_VERSION"
        echo "Please update OBS Studio from https://obsproject.com"
    fi
fi

# Remove old plugin if it exists (user directory)
# Determine user home directory robustly
if [ -n "$HOME" ]; then
    USER_HOME="$HOME"
elif [ -n "$USER" ]; then
    USER_HOME="/Users/$USER"
else
    echo "WARNING: Could not determine user home directory. Skipping user plugin removal."
    USER_HOME=""
fi

if [ -n "$USER_HOME" ]; then
    OLD_PLUGIN="$USER_HOME/Library/Application Support/obs-studio/plugins/obs-polyemesis.plugin"
    if [ -d "$OLD_PLUGIN" ]; then
        echo "Removing previous installation..."
        rm -rf "$OLD_PLUGIN"
    fi
fi
# Also check system-wide location (old installation path)
OLD_SYSTEM_PLUGIN="/Library/Application Support/obs-studio/plugins/obs-polyemesis.plugin"
if [ -d "$OLD_SYSTEM_PLUGIN" ]; then
    echo "Removing old system-wide installation..."
    rm -rf "$OLD_SYSTEM_PLUGIN"
fi

exit 0
EOF

    chmod +x "$PKG_DIR/scripts/preinstall"
    log_info "Preinstall script created"
}

# Create postinstall script
create_postinstall_script() {
    log_info "Creating postinstall script..."

    cat > "$PKG_DIR/scripts/postinstall" << 'EOF'
#!/bin/bash
# Post-installation script for OBS Polyemesis

echo "OBS Polyemesis has been successfully installed!"
echo ""
echo "To use the plugin:"
echo "1. Launch OBS Studio"
echo "2. Go to View → Docks → Restreamer Control"
echo "3. Configure your datarhei Restreamer connection"
echo ""
echo "For documentation, visit:"
echo "https://github.com/rainmanjam/obs-polyemesis"

exit 0
EOF

    chmod +x "$PKG_DIR/scripts/postinstall"
    log_info "Postinstall script created"
}

# Create welcome text
create_welcome() {
    log_info "Creating welcome text..."

    cat > "$PKG_DIR/resources/welcome.rtf" << 'EOF'
{\rtf1\ansi\ansicpg1252\cocoartf2709
\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\fswiss\fcharset0 Helvetica;\f1\fswiss\fcharset0 Helvetica-Bold;}
{\colortbl;\red255\green255\blue255;\red0\green0\blue0;}
{\*\expandedcolortbl;;\cssrgb\c0\c0\c0;}
\margl1440\margr1440\vieww11520\viewh8400\viewkind0
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\fs28 \cf2 This installer will install the OBS Polyemesis plugin for controlling datarhei Restreamer.\
\
\pard\tx220\tx720\pardeftab720\li720\fi-720\partightenfactor0
\ls1\ilvl0
\f1\b \cf2 {\listtext	\uc0\u8226 	}
\f0\b0 OBS Studio 28.0 or later\
{\listtext	\uc0\u8226 	}macOS 11.0 (Big Sur) or later\
{\listtext	\uc0\u8226 	}datarhei Restreamer instance (local or remote)\
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0
\cf2 \
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f1\b \cf2 Features:\
\pard\tx220\tx720\pardeftab720\li720\fi-720\partightenfactor0
\ls2\ilvl0
\f0\b0 \cf2 {\listtext	\uc0\u8226 	}Full Restreamer process control\
{\listtext	\uc0\u8226 	}Real-time monitoring and statistics\
{\listtext	\uc0\u8226 	}Advanced multistreaming with orientation detection\
{\listtext	\uc0\u8226 	}Support for Twitch, YouTube, TikTok, Instagram, and more\
}
EOF

    log_info "Welcome text created"
}

# Create README
create_readme() {
    log_info "Creating README..."

    cat > "$PKG_DIR/resources/readme.rtf" << 'EOF'
{\rtf1\ansi\ansicpg1252\cocoartf2709
\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\fswiss\fcharset0 Helvetica;\f1\fswiss\fcharset0 Helvetica-Bold;\f2\fmodern\fcharset0 Courier;}
{\colortbl;\red255\green255\blue255;\red0\green0\blue0;\red0\green0\blue238;}
{\*\expandedcolortbl;;\cssrgb\c0\c0\c0;\cssrgb\c0\c0\c93333;}
\margl1440\margr1440\vieww11520\viewh8400\viewkind0
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\fs28 \cf2 \
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f1\b\fs32 \cf2 Installation\
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\b0\fs28 \cf2 \
This package will install the OBS Polyemesis plugin to:\
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f2\fs24 \cf2 ~/Library/Application Support/obs-studio/plugins/\
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\fs28 \cf2 \
The plugin will be installed for your user account.\
\
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f1\b\fs32 \cf2 First Use\
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\b0\fs28 \cf2 \
After installation:\
\pard\tx220\tx720\pardeftab720\li720\fi-720\partightenfactor0
\ls1\ilvl0\cf2 {\listtext	1.	}Launch OBS Studio\
{\listtext	2.	}Go to View \'97 Docks \'97 Restreamer Control\
{\listtext	3.	}Configure your Restreamer connection (host, port)\
{\listtext	4.	}Click "Test Connection"\
{\listtext	5.	}Start managing your streams!\
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0
\cf2 \
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f1\b\fs32 \cf2 Documentation\
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\b0\fs28 \cf2 \
Full documentation: {\field{\*\fldinst{HYPERLINK "https://github.com/rainmanjam/obs-polyemesis"}}{\fldrslt \cf3 \ul \ulc3 github.com/rainmanjam/obs-polyemesis}}\
\
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f1\b\fs32 \cf2 Support\
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\b0\fs28 \cf2 \
For issues and questions: {\field{\*\fldinst{HYPERLINK "https://github.com/rainmanjam/obs-polyemesis/issues"}}{\fldrslt \cf3 \ul \ulc3 github.com/rainmanjam/obs-polyemesis/issues}}\
}
EOF

    log_info "README created"
}

# Build component package
build_component_package() {
    log_info "Building component package..."

    pkgbuild \
        --root "$PKG_DIR/root" \
        --identifier "$IDENTIFIER" \
        --version "$VERSION" \
        --scripts "$PKG_DIR/scripts" \
        --install-location "/" \
        "$PKG_DIR/${PLUGIN_NAME}-component.pkg"

    log_info "Component package built"
}

# Create distribution XML
create_distribution_xml() {
    log_info "Creating distribution XML..."

    cat > "$PKG_DIR/distribution.xml" << EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>OBS Polyemesis ${VERSION}</title>
    <organization>com.rainmanjam</organization>
    <domains enable_anywhere="false" enable_currentUserHome="true" enable_localSystem="false"/>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>

    <!-- Welcome and README -->
    <welcome file="welcome.rtf" mime-type="text/rtf"/>
    <readme file="readme.rtf" mime-type="text/rtf"/>
    <license file="../../LICENSE"/>

    <!-- Define installation check -->
    <installation-check script="installationCheck()"/>

    <script>
    <![CDATA[
    function installationCheck() {
        if (system.compareVersions(system.version.ProductVersion, '11.0') < 0) {
            my.result.title = 'Unable to Install';
            my.result.message = 'OBS Polyemesis requires macOS 11.0 (Big Sur) or later.';
            my.result.type = 'Fatal';
            return false;
        }
        return true;
    }
    ]]>
    </script>

    <choices-outline>
        <line choice="default">
            <line choice="${IDENTIFIER}"/>
        </line>
    </choices-outline>

    <choice id="default"/>
    <choice id="${IDENTIFIER}" visible="false">
        <pkg-ref id="${IDENTIFIER}"/>
    </choice>

    <pkg-ref id="${IDENTIFIER}" version="${VERSION}" onConclusion="none">
        ${PLUGIN_NAME}-component.pkg
    </pkg-ref>
</installer-gui-script>
EOF

    log_info "Distribution XML created"
}

# Build product archive
build_product() {
    log_info "Building final product..."

    OUTPUT_PKG="$PROJECT_ROOT/${PLUGIN_NAME}-${VERSION}-macos.pkg"

    productbuild \
        --distribution "$PKG_DIR/distribution.xml" \
        --resources "$PKG_DIR/resources" \
        --package-path "$PKG_DIR" \
        "$OUTPUT_PKG"

    log_info "Product built successfully: $OUTPUT_PKG"
}

# Optional: Sign the package
sign_package() {
    if [ -n "$SIGNING_IDENTITY" ]; then
        log_info "Signing package with identity: $SIGNING_IDENTITY"

        OUTPUT_PKG="$PROJECT_ROOT/${PLUGIN_NAME}-${VERSION}-macos.pkg"
        SIGNED_PKG="$PROJECT_ROOT/${PLUGIN_NAME}-${VERSION}-macos-signed.pkg"

        productsign \
            --sign "$SIGNING_IDENTITY" \
            "$OUTPUT_PKG" \
            "$SIGNED_PKG"

        mv "$SIGNED_PKG" "$OUTPUT_PKG"
        log_info "Package signed successfully"
    else
        log_warn "No signing identity specified (SIGNING_IDENTITY). Package will be unsigned."
        log_warn "For distribution, set SIGNING_IDENTITY environment variable."
    fi
}

# Display package info
display_info() {
    OUTPUT_PKG="$PROJECT_ROOT/${PLUGIN_NAME}-${VERSION}-macos.pkg"

    log_info "Package created successfully!"
    echo ""
    echo "Package Information:"
    echo "  Name:     ${PLUGIN_NAME}-${VERSION}-macos.pkg"
    echo "  Location: $OUTPUT_PKG"
    echo "  Size:     $(du -h "$OUTPUT_PKG" | cut -f1)"
    echo "  Version:  $VERSION"
    echo ""
    echo "To install:"
    echo "  sudo installer -pkg \"$OUTPUT_PKG\" -target /"
    echo ""
    echo "Or double-click the .pkg file to install via Installer.app"
}

# Main execution
main() {
    log_info "Starting macOS package build for OBS Polyemesis $VERSION"

    check_prerequisites
    verify_plugin
    create_package_structure
    create_preinstall_script
    create_postinstall_script
    create_welcome
    create_readme
    build_component_package
    create_distribution_xml
    build_product
    sign_package
    display_info

    log_info "Package build complete!"
}

# Run main function
main "$@"
