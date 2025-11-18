#!/bin/bash
# Run tests on Windows machine
# Usage: ./scripts/windows-test.sh [test-type]

set -e

# Configuration
WINDOWS_HOST="${WINDOWS_ACT_HOST:-windows-act}"
WORKSPACE_PATH="${WINDOWS_WORKSPACE:-C:/Users/rainm/Documents/GitHub/obs-polyemesis}"
VERBOSE="${VERBOSE:-0}"
TEST_WORKFLOW=".github/workflows/test.yaml"

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
Usage: $0 [test-type] [options]

Run test suite on Windows machine.

Arguments:
    test-type               Type of tests to run (all, unit, integration, api)
                            Default: all

Options:
    -h, --help              Show this help message
    --host HOST             Windows SSH host (default: windows-act)
    --workspace PATH        Workspace path on Windows (default: C:/act-workspace/obs-polyemesis)
    --workflow FILE         Test workflow file (default: .github/workflows/test.yaml)
    -v, --verbose           Verbose output

Examples:
    # Run all tests
    $0

    # Run unit tests only
    $0 unit

    # Run integration tests only
    $0 integration

    # Run API integration tests
    $0 api

    # Run with custom workflow
    $0 --workflow .github/workflows/custom-test.yaml

EOF
    exit 0
}

# Parse arguments
TEST_TYPE="all"
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
        --workflow)
            TEST_WORKFLOW="$2"
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
            TEST_TYPE="$1"
            shift
            ;;
    esac
done

# Validate test type
case "$TEST_TYPE" in
    all|unit|integration|api)
        ;;
    *)
        log_error "Invalid test type: $TEST_TYPE"
        log_info "Valid types: all, unit, integration, api"
        exit 1
        ;;
esac

# Check SSH connection
log_info "Checking SSH connection to Windows..."
if ! ssh -q -o BatchMode=yes -o ConnectTimeout=5 "$WINDOWS_HOST" exit 2>/dev/null; then
    log_error "Cannot connect to Windows host: $WINDOWS_HOST"
    exit 1
fi
log_info "✓ Connected to $WINDOWS_HOST"

# Determine which job to run based on test type
case "$TEST_TYPE" in
    all)
        TEST_JOB=""
        log_info "Running all test jobs"
        ;;
    unit)
        TEST_JOB="unit-tests"
        log_info "Running unit tests only"
        ;;
    integration)
        TEST_JOB="integration-tests"
        log_info "Running integration tests only"
        ;;
    api)
        TEST_JOB="api-integration-tests"
        log_info "Running API integration tests only"
        ;;
esac

# Build act command
ACT_CMD="cd \"$WORKSPACE_PATH\" && act -W \"$TEST_WORKFLOW\""
if [ -n "$TEST_JOB" ]; then
    ACT_CMD="$ACT_CMD -j \"$TEST_JOB\""
fi

log_debug "Command: $ACT_CMD"
log_info "Starting tests on Windows..."
echo ""

# Execute tests via SSH
ssh -t "$WINDOWS_HOST" "$ACT_CMD"
EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    log_info "✓ Tests passed successfully"

    # Fetch test results if available
    log_info "Checking for test results..."
    ssh "$WINDOWS_HOST" bash << 'EOF_RESULTS'
if [ -f "$WORKSPACE_PATH/test-results.xml" ]; then
    echo "Test results available at: $WORKSPACE_PATH/test-results.xml"
elif [ -f "$WORKSPACE_PATH/build/test-results.xml" ]; then
    echo "Test results available at: $WORKSPACE_PATH/build/test-results.xml"
fi

if [ -f "$WORKSPACE_PATH/coverage/coverage.html" ]; then
    echo "Coverage report available at: $WORKSPACE_PATH/coverage/coverage.html"
fi
EOF_RESULTS

    log_info ""
    log_info "To copy test results to Mac:"
    log_info "  scp '$WINDOWS_HOST:$WORKSPACE_PATH/test-results.xml' ~/Downloads/"
    log_info "  scp -r '$WINDOWS_HOST:$WORKSPACE_PATH/coverage/' ~/Downloads/"
else
    log_error "✗ Tests failed with exit code $EXIT_CODE"

    # Try to display recent test logs
    log_info ""
    log_info "Recent test output:"
    ssh "$WINDOWS_HOST" bash << 'EOF_LOGS'
if [ -f "$WORKSPACE_PATH/test.log" ]; then
    tail -50 "$WORKSPACE_PATH/test.log"
elif [ -f "$WORKSPACE_PATH/build/test.log" ]; then
    tail -50 "$WORKSPACE_PATH/build/test.log"
else
    echo "No test log file found"
fi
EOF_LOGS
fi

exit $EXIT_CODE
