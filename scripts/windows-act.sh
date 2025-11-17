#!/bin/bash
# Windows act wrapper - Execute act commands on Windows machine from Mac
# Usage: ./scripts/windows-act.sh [act arguments]

set -e

# Configuration
WINDOWS_HOST="${WINDOWS_ACT_HOST:-windows-act}"
WORKSPACE_PATH="${WINDOWS_WORKSPACE:-C:/Users/rainm/Documents/GitHub/obs-polyemesis}"
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

# Check if SSH is configured
check_ssh() {
    if ! ssh -q -o BatchMode=yes -o ConnectTimeout=5 "$WINDOWS_HOST" exit 2>/dev/null; then
        log_error "Cannot connect to Windows host: $WINDOWS_HOST"
        log_info "Please ensure:"
        log_info "  1. Windows machine is running and accessible"
        log_info "  2. SSH is configured in ~/.ssh/config with host 'windows-act'"
        log_info "  3. SSH keys are properly set up"
        log_info ""
        log_info "Test connection with: ssh $WINDOWS_HOST"
        exit 1
    fi
    log_debug "SSH connection to $WINDOWS_HOST verified"
}

# Display usage
usage() {
    cat << EOF
Usage: $0 [options] [act arguments]

Execute act commands on Windows machine from Mac.

Options:
    -h, --help              Show this help message
    --host HOST             Windows SSH host (default: windows-act)
    --workspace PATH        Workspace path on Windows (default: C:/act-workspace/obs-polyemesis)
    -v, --verbose           Verbose output

Environment Variables:
    WINDOWS_ACT_HOST        Override default Windows host
    WINDOWS_WORKSPACE       Override default workspace path
    VERBOSE                 Enable verbose output (1 or 0)

Examples:
    # List all workflows
    $0 -l

    # Run specific workflow
    $0 -W .github/workflows/create-packages.yaml

    # Run specific job
    $0 -W .github/workflows/create-packages.yaml -j build-windows

    # Dry run
    $0 -W .github/workflows/create-packages.yaml -n

    # Run with secrets
    $0 -W .github/workflows/test.yaml -s GITHUB_TOKEN=abc123

    # Use custom Windows host
    $0 --host windows-act-2 -l

EOF
    exit 0
}

# Parse script options (before act arguments)
SCRIPT_ARGS=()
ACT_ARGS=()
PARSE_SCRIPT_ARGS=1

while [ $# -gt 0 ]; do
    if [ "$PARSE_SCRIPT_ARGS" = "1" ]; then
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
            -v|--verbose)
                VERBOSE=1
                shift
                ;;
            --)
                PARSE_SCRIPT_ARGS=0
                shift
                ;;
            *)
                # Unknown option, pass to act
                PARSE_SCRIPT_ARGS=0
                ACT_ARGS+=("$1")
                shift
                ;;
        esac
    else
        ACT_ARGS+=("$1")
        shift
    fi
done

# Main execution
main() {
    log_info "Windows act Wrapper"
    log_debug "Host: $WINDOWS_HOST"
    log_debug "Workspace: $WORKSPACE_PATH"

    # Check SSH connection
    log_info "Checking SSH connection to Windows..."
    check_ssh
    log_info "✓ Connected to $WINDOWS_HOST"

    # Build act command
    ACT_CMD="cd \"$WORKSPACE_PATH\" && act"

    # Add act arguments
    for arg in "${ACT_ARGS[@]}"; do
        # Escape quotes in arguments
        escaped_arg="${arg//\"/\\\"}"
        ACT_CMD="$ACT_CMD \"$escaped_arg\""
    done

    log_debug "Command: $ACT_CMD"
    log_info "Executing act on Windows..."
    echo ""

    # Execute via SSH (interactive to preserve output streaming)
    ssh -t "$WINDOWS_HOST" "$ACT_CMD"
    EXIT_CODE=$?

    echo ""
    if [ $EXIT_CODE -eq 0 ]; then
        log_info "✓ act execution completed successfully"
    else
        log_error "✗ act execution failed with exit code $EXIT_CODE"
    fi

    exit $EXIT_CODE
}

main

