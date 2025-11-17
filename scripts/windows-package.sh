#!/bin/bash
# Create Windows installer package
# Usage: ./scripts/windows-package.sh [options]

set -e

# Configuration
WINDOWS_HOST="${WINDOWS_ACT_HOST:-windows-act}"
WORKSPACE_PATH="${WINDOWS_WORKSPACE:-C:/Users/rainm/Documents/GitHub/obs-polyemesis}"
VERBOSE="${VERBOSE:-0}"
FETCH_PACKAGE=0
DOWNLOAD_DIR="$HOME/Downloads"
VERSION=""
PACKAGE_WORKFLOW=".github/workflows/create-packages.yaml"
PACKAGE_JOB="package-windows"

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

Create Windows installer package on Windows machine.

Options:
    -h, --help              Show this help message
    --host HOST             Windows SSH host (default: windows-act)
    --workspace PATH        Workspace path on Windows (default: C:/act-workspace/obs-polyemesis)
    --fetch                 Copy installer to Mac after building
    --download-dir DIR      Directory to save installer (default: ~/Downloads)
    --version VERSION       Package version (default: auto-detect from git)
    --workflow FILE         Packaging workflow (default: .github/workflows/create-packages.yaml)
    --job JOB               Packaging job (default: package-windows)
    -v, --verbose           Verbose output

Examples:
    # Create installer with default settings
    $0

    # Create installer and copy to Mac
    $0 --fetch

    # Create installer with specific version
    $0 --version 0.9.1

    # Create and fetch to custom directory
    $0 --fetch --download-dir ~/Desktop

EOF
    exit 0
}

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            usage
            ;;
        --host)
            WINDOWS_HOST="$2"
            shift 2
            ;;
        --workspace)
            WORKSPACE_PATH="$2"
            shift 2
            ;;
        --fetch)
            FETCH_PACKAGE=1
            shift
            ;;
        --download-dir)
            DOWNLOAD_DIR="$2"
            shift 2
            ;;
        --version)
            VERSION="$2"
            shift 2
            ;;
        --workflow)
            PACKAGE_WORKFLOW="$2"
            shift 2
            ;;
        --job)
            PACKAGE_JOB="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -*)
            log_error "Unknown option: $1"
            usage
            ;;
        *)
            log_error "Unexpected argument: $1"
            usage
            ;;
    esac
done

# Auto-detect version if not specified
if [ -z "$VERSION" ]; then
    if git rev-parse --git-dir > /dev/null 2>&1; then
        # Try to get version from git tag
        VERSION=$(git describe --tags --abbrev=0 2>/dev/null || echo "")

        if [ -z "$VERSION" ]; then
            # Fallback to commit hash
            VERSION=$(git rev-parse --short HEAD)
            log_debug "No version tag found, using commit hash: $VERSION"
        else
            log_debug "Detected version from git tag: $VERSION"
        fi
    else
        log_warn "Not in a git repository, version will be determined by build"
        VERSION="unknown"
    fi
fi

# Check SSH connection
log_info "Checking SSH connection to Windows..."
if ! ssh -q -o BatchMode=yes -o ConnectTimeout=5 "$WINDOWS_HOST" exit 2>/dev/null; then
    log_error "Cannot connect to Windows host: $WINDOWS_HOST"
    exit 1
fi
log_info "✓ Connected to $WINDOWS_HOST"

# Run packaging workflow
log_info "Creating Windows installer package..."
log_debug "Workflow: $PACKAGE_WORKFLOW"
log_debug "Job: $PACKAGE_JOB"
log_debug "Version: $VERSION"

echo ""
ssh -t "$WINDOWS_HOST" bash << EOF
cd "$WORKSPACE_PATH"
echo "=== Running packaging workflow ==="

# Set version environment variable if specified
export PACKAGE_VERSION="$VERSION"

# Run act packaging workflow
act -W "$PACKAGE_WORKFLOW" -j "$PACKAGE_JOB" \
    -s PACKAGE_VERSION="$VERSION"
EOF

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -ne 0 ]; then
    log_error "✗ Packaging failed with exit code $EXIT_CODE"
    exit $EXIT_CODE
fi

log_info "✓ Packaging completed successfully"

# Find the installer file on Windows
log_info "Locating installer file..."
INSTALLER_PATH=$(ssh "$WINDOWS_HOST" bash << 'EOF'
cd "$WORKSPACE_PATH"

# Common installer locations
SEARCH_PATHS=(
    "build/obs-polyemesis-installer.exe"
    "build/Release/obs-polyemesis-installer.exe"
    "dist/obs-polyemesis-installer.exe"
    "build/obs-polyemesis-*.exe"
)

for path in "${SEARCH_PATHS[@]}"; do
    FOUND=$(find . -path "./$path" -type f 2>/dev/null | head -1)
    if [ -n "$FOUND" ]; then
        echo "$FOUND"
        exit 0
    fi
done

# Fallback: search entire build directory
find ./build -name "*.exe" -type f 2>/dev/null | grep -i installer | head -1
EOF
)

if [ -z "$INSTALLER_PATH" ]; then
    log_error "Could not locate installer file on Windows"
    log_info "Checking common locations..."

    ssh "$WINDOWS_HOST" bash << EOF
cd "$WORKSPACE_PATH"
echo "Contents of build directory:"
ls -lh build/ 2>/dev/null || echo "build/ directory not found"
echo ""
echo "Searching for .exe files:"
find . -name "*.exe" -type f 2>/dev/null | head -10
EOF

    exit 1
fi

# Clean up path (remove ./ prefix)
INSTALLER_PATH="${INSTALLER_PATH#./}"
INSTALLER_FILENAME=$(basename "$INSTALLER_PATH")

log_info "✓ Found installer: $INSTALLER_FILENAME"

# Display installer info
log_info ""
log_info "Installer Information:"
ssh "$WINDOWS_HOST" bash << EOF
cd "$WORKSPACE_PATH"
INSTALLER="$INSTALLER_PATH"

if [ -f "\$INSTALLER" ]; then
    echo "  Path: $WORKSPACE_PATH/$INSTALLER_PATH"
    SIZE=\$(du -h "\$INSTALLER" | cut -f1)
    echo "  Size: \$SIZE"
    echo "  Created: \$(stat -c %y "\$INSTALLER" 2>/dev/null || stat -f "%Sm" "\$INSTALLER")"
else
    echo "  Error: File not found at \$INSTALLER"
    exit 1
fi
EOF

# Fetch package if requested
if [ $FETCH_PACKAGE -eq 1 ]; then
    log_info ""
    log_info "Copying installer to Mac..."

    # Ensure download directory exists
    mkdir -p "$DOWNLOAD_DIR"

    # Add version to filename if not already present
    if [[ "$INSTALLER_FILENAME" != *"$VERSION"* ]] && [ "$VERSION" != "unknown" ]; then
        # Insert version before .exe extension
        NEW_FILENAME="${INSTALLER_FILENAME%.exe}-${VERSION}.exe"
    else
        NEW_FILENAME="$INSTALLER_FILENAME"
    fi

    LOCAL_PATH="$DOWNLOAD_DIR/$NEW_FILENAME"

    scp "$WINDOWS_HOST:$WORKSPACE_PATH/$INSTALLER_PATH" "$LOCAL_PATH"

    log_info "✓ Installer copied to: $LOCAL_PATH"

    # Display file info
    if [ -f "$LOCAL_PATH" ]; then
        FILE_SIZE=$(du -h "$LOCAL_PATH" | cut -f1)
        log_info ""
        log_info "Downloaded Installer:"
        log_info "  Location: $LOCAL_PATH"
        log_info "  Size: $FILE_SIZE"

        # Open download directory (macOS only)
        if command -v open >/dev/null 2>&1; then
            log_info ""
            log_info "Opening download directory..."
            open "$DOWNLOAD_DIR"
        fi
    else
        log_error "File copy failed"
        exit 1
    fi
else
    log_info ""
    log_info "Installer available on Windows at:"
    log_info "  $WORKSPACE_PATH/$INSTALLER_PATH"
    log_info ""
    log_info "To copy to Mac, run:"
    log_info "  scp '$WINDOWS_HOST:$WORKSPACE_PATH/$INSTALLER_PATH' ~/Downloads/"
    log_info ""
    log_info "Or run this script with --fetch option"
fi

log_info ""
log_info "✓ Packaging process completed successfully"

exit 0
