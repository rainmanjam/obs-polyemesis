#!/bin/bash
# Sync code to Windows and run build
# Usage: ./scripts/sync-and-build-windows.sh [branch] [options]

set -e

# Configuration
WINDOWS_HOST="${WINDOWS_ACT_HOST:-windows-act}"
WORKSPACE_PATH="${WINDOWS_WORKSPACE:-C:/Users/rainm/Documents/GitHub/obs-polyemesis}"
VERBOSE="${VERBOSE:-0}"
SYNC_ONLY=0
BUILD_WORKFLOW=".github/workflows/create-packages.yaml"
BUILD_JOB="windows-package"

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
Usage: $0 [branch] [options]

Sync local repository to Windows machine and run build.

Arguments:
    branch                  Branch to sync (default: current branch)

Options:
    -h, --help              Show this help message
    --host HOST             Windows SSH host (default: windows-act)
    --workspace PATH        Workspace path on Windows (default: C:/act-workspace/obs-polyemesis)
    --sync-only             Only sync files, don't build
    --workflow FILE         Workflow file to run (default: .github/workflows/create-packages.yaml)
    --job JOB               Job to run (default: build-windows)
    -v, --verbose           Verbose output

Examples:
    # Sync and build current branch
    $0

    # Sync and build specific branch
    $0 feature/new-feature

    # Sync only, no build
    $0 --sync-only

    # Sync and build with custom workflow
    $0 --workflow .github/workflows/custom.yaml --job custom-build

EOF
    exit 0
}

# Parse arguments
BRANCH=""
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
        --sync-only)
            SYNC_ONLY=1
            shift
            ;;
        --workflow)
            BUILD_WORKFLOW="$2"
            shift 2
            ;;
        --job)
            BUILD_JOB="$2"
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
            BRANCH="$1"
            shift
            ;;
    esac
done

# Get current branch if not specified
if [ -z "$BRANCH" ]; then
    BRANCH=$(git branch --show-current)
    log_debug "Using current branch: $BRANCH"
fi

# Verify we're in a git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    log_error "Not in a git repository"
    exit 1
fi

# Check for uncommitted changes
if ! git diff-index --quiet HEAD --; then
    log_warn "You have uncommitted changes. These will be synced to Windows."
    log_warn "Consider committing or stashing them first."

    # Only prompt if running interactively
    if [ -t 0 ]; then
        read -p "Continue anyway? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            log_info "Sync cancelled"
            exit 0
        fi
    else
        log_info "Running non-interactively, proceeding with sync..."
    fi
fi

# Check SSH connection
log_info "Checking SSH connection to Windows..."
if ! ssh -q -o BatchMode=yes -o ConnectTimeout=5 "$WINDOWS_HOST" exit 2>/dev/null; then
    log_error "Cannot connect to Windows host: $WINDOWS_HOST"
    exit 1
fi
log_info "✓ Connected to $WINDOWS_HOST"

# Sync files using rsync
log_info "Syncing repository to Windows..."
log_debug "Source: $(pwd)"
log_debug "Destination: $WINDOWS_HOST:$WORKSPACE_PATH"

# Build rsync exclusion list
RSYNC_EXCLUDES=(
    --exclude='.git/objects'
    --exclude='build/'
    --exclude='dist/'
    --exclude='.DS_Store'
    --exclude='*.pyc'
    --exclude='__pycache__/'
    --exclude='node_modules/'
    --exclude='.vscode/'
    --exclude='.idea/'
)

# Check if rsync is available on both local and remote
if command -v rsync >/dev/null 2>&1 && ssh "$WINDOWS_HOST" "command -v rsync" >/dev/null 2>&1; then
    log_debug "Using rsync for sync"

    RSYNC_OPTS="-avz --delete"
    if [ "$VERBOSE" != "1" ]; then
        RSYNC_OPTS="-az --delete --progress"
    fi

    # shellcheck disable=SC2086
    rsync $RSYNC_OPTS \
        "${RSYNC_EXCLUDES[@]}" \
        ./ "$WINDOWS_HOST:$WORKSPACE_PATH/"

    log_info "✓ Files synced successfully"
else
    log_warn "rsync not found, using scp (slower)"

    # Create tarball and transfer
    TEMP_TAR="/tmp/obs-polyemesis-sync-$$.tar.gz"
    REMOTE_TAR="C:/Users/rainm/obs-polyemesis-sync.tar.gz"
    log_debug "Creating tarball: $TEMP_TAR"

    tar czf "$TEMP_TAR" \
        --exclude='.git/objects' \
        --exclude='build' \
        --exclude='dist' \
        --exclude='.DS_Store' \
        .

    scp "$TEMP_TAR" "$WINDOWS_HOST:$REMOTE_TAR"
    ssh "$WINDOWS_HOST" "powershell -Command \"New-Item -ItemType Directory -Force -Path '${WORKSPACE_PATH}' | Out-Null; Set-Location '${WORKSPACE_PATH}'; tar -xzf '${REMOTE_TAR}'; Remove-Item '${REMOTE_TAR}'\""
    rm "$TEMP_TAR"

    log_info "✓ Files synced successfully"
fi

# Update .git metadata if needed
log_info "Updating Git metadata on Windows..."
# Convert Windows path to WSL path (C:/Users/... -> /mnt/c/Users/...)
WSL_PATH=$(echo "$WORKSPACE_PATH" | sed 's|C:/|/mnt/c/|' | sed 's|\\|/|g')
ssh "$WINDOWS_HOST" bash << 'EOF_GIT'
cd "${WSL_PATH}"

# Initialize git if needed
if [ ! -d .git ]; then
    git init
    git remote add origin https://github.com/rainmanjam/obs-polyemesis.git 2>/dev/null || true
fi

# Checkout branch
git checkout -B "${BRANCH}" 2>/dev/null || true

echo "✓ Git metadata updated"
EOF_GIT
log_info "✓ Git metadata updated"

# If sync-only, exit here
if [ $SYNC_ONLY -eq 1 ]; then
    log_info "✓ Sync completed (build skipped)"
    exit 0
fi

# Run build on Windows
log_info "Starting build on Windows..."
log_debug "Workflow: $BUILD_WORKFLOW"
log_debug "Job: $BUILD_JOB"

echo ""
ssh -t "$WINDOWS_HOST" bash << 'EOF_BUILD'
cd "${WSL_PATH}"
echo "=== Running act build ==="
/mnt/c/Users/rainm/AppData/Local/Microsoft/WinGet/Links/act.exe -W "${BUILD_WORKFLOW}" -j "${BUILD_JOB}"
EOF_BUILD

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    log_info "✓ Build completed successfully"
    log_info ""
    log_info "Build artifacts are available on Windows at:"
    log_info "  $WORKSPACE_PATH/build/"
    log_info ""
    log_info "To copy artifacts to Mac:"
    log_info "  scp '$WINDOWS_HOST:$WORKSPACE_PATH/build/*.exe' ~/Downloads/"
else
    log_error "✗ Build failed with exit code $EXIT_CODE"
    log_info ""
    log_info "To view build logs:"
    log_info "  ssh $WINDOWS_HOST 'cat $WORKSPACE_PATH/build/build.log'"
fi

exit $EXIT_CODE
