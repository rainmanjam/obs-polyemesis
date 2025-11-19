#!/usr/bin/env bash
# Build OBS Polyemesis for all platforms
# Usage: ./scripts/build-all-platforms.sh [options]
# Requires: bash 4.0+ for associative arrays

set -e

# Check bash version
if [ "${BASH_VERSINFO[0]}" -lt 4 ]; then
    echo "Error: This script requires bash 4.0 or later"
    echo "You have: bash ${BASH_VERSION}"
    echo ""
    echo "On macOS, install via Homebrew:"
    echo "  brew install bash"
    echo ""
    echo "Current bash locations:"
    echo "  System: /bin/bash ($( (/bin/bash --version 2>&1 || echo "version unknown") | head -1 ))"
    echo "  Active: $(which bash) ($(bash --version | head -1))"
    exit 1
fi

# Configuration
BUILD_MACOS=1
BUILD_LINUX=1
BUILD_WINDOWS=1
STOP_ON_ERROR=1
VERBOSE="${VERBOSE:-0}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
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

log_platform() {
    echo -e "${CYAN}[PLATFORM]${NC} $1"
}

# Display usage
usage() {
    cat << EOF
Usage: $0 [options]

Build OBS Polyemesis for all platforms sequentially.

This script builds:
  1. macOS (local build via scripts/macos-build.sh)
  2. Linux (local Docker build via act)
  3. Windows (remote build via scripts/sync-and-build-windows.sh)

Options:
    -h, --help              Show this help message
    --skip-macos            Skip macOS build
    --skip-linux            Skip Linux build
    --skip-windows          Skip Windows build
    --no-stop-on-error      Continue even if a platform build fails
    --build-type TYPE       Build type: Release, Debug (default: Release)
    -v, --verbose           Verbose output

Environment Variables:
    VERBOSE                 Enable verbose output (1 or 0)
    BUILD_TYPE              Build type

Examples:
    # Build all platforms
    $0

    # Build only macOS and Linux
    $0 --skip-windows

    # Continue on error
    $0 --no-stop-on-error

    # Debug build for all platforms
    $0 --build-type Debug

EOF
    exit 0
}

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            usage
            ;;
        --skip-macos)
            BUILD_MACOS=0
            shift
            ;;
        --skip-linux)
            BUILD_LINUX=0
            shift
            ;;
        --skip-windows)
            BUILD_WINDOWS=0
            shift
            ;;
        --no-stop-on-error)
            STOP_ON_ERROR=0
            shift
            ;;
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            export VERBOSE
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Track results
declare -A BUILD_RESULTS
declare -A BUILD_TIMES

# Main execution
main() {
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  OBS Polyemesis - Multi-Platform Build"
    echo "══════════════════════════════════════════════════════════════"
    echo ""

    log_info "Build configuration:"
    log_info "  Build type: $BUILD_TYPE"
    log_info "  Stop on error: $([ $STOP_ON_ERROR -eq 1 ] && echo 'Yes' || echo 'No')"
    log_info ""

    PLATFORMS_TO_BUILD=()
    [ $BUILD_MACOS -eq 1 ] && PLATFORMS_TO_BUILD+=("macOS")
    [ $BUILD_LINUX -eq 1 ] && PLATFORMS_TO_BUILD+=("Linux")
    [ $BUILD_WINDOWS -eq 1 ] && PLATFORMS_TO_BUILD+=("Windows")

    if [ ${#PLATFORMS_TO_BUILD[@]} -eq 0 ]; then
        log_error "No platforms selected for build"
        exit 1
    fi

    log_info "Platforms to build: ${PLATFORMS_TO_BUILD[*]}"
    echo ""

    TOTAL_START=$(date +%s)

    # ==========================================
    # macOS Build
    # ==========================================
    if [ $BUILD_MACOS -eq 1 ]; then
        echo ""
        echo "────────────────────────────────────────────────────────────"
        log_platform "Building for macOS (1/${#PLATFORMS_TO_BUILD[@]})"
        echo "────────────────────────────────────────────────────────────"
        echo ""

        PLATFORM_START=$(date +%s)

        if BUILD_TYPE="$BUILD_TYPE" ./scripts/macos-build.sh; then
            PLATFORM_END=$(date +%s)
            BUILD_DURATION=$((PLATFORM_END - PLATFORM_START))
            BUILD_RESULTS[macOS]="✓ Success"
            BUILD_TIMES[macOS]=$BUILD_DURATION

            log_info "✓ macOS build completed in ${BUILD_DURATION}s"
        else
            PLATFORM_END=$(date +%s)
            BUILD_DURATION=$((PLATFORM_END - PLATFORM_START))
            BUILD_RESULTS[macOS]="✗ Failed"
            BUILD_TIMES[macOS]=$BUILD_DURATION

            log_error "✗ macOS build failed after ${BUILD_DURATION}s"

            if [ $STOP_ON_ERROR -eq 1 ]; then
                echo ""
                log_error "Stopping build due to macOS failure"
                display_summary
                exit 1
            fi
        fi
    fi

    # ==========================================
    # Linux Build
    # ==========================================
    if [ $BUILD_LINUX -eq 1 ]; then
        echo ""
        echo "────────────────────────────────────────────────────────────"
        PLATFORM_NUM=1
        [ $BUILD_MACOS -eq 1 ] && PLATFORM_NUM=2
        log_platform "Building for Linux ($PLATFORM_NUM/${#PLATFORMS_TO_BUILD[@]})"
        echo "────────────────────────────────────────────────────────────"
        echo ""

        PLATFORM_START=$(date +%s)

        # Use act to build Linux in Docker
        log_info "Running Linux build via act (Docker)..."

        ACT_ARGS=(
            -W .github/workflows/automated-tests.yml
            -j build-linux
            --container-architecture linux/amd64
        )

        if [ "$VERBOSE" = "1" ]; then
            ACT_ARGS+=(--verbose)
        fi

        if act "${ACT_ARGS[@]}"; then
            PLATFORM_END=$(date +%s)
            BUILD_DURATION=$((PLATFORM_END - PLATFORM_START))
            BUILD_RESULTS[Linux]="✓ Success"
            BUILD_TIMES[Linux]=$BUILD_DURATION

            log_info "✓ Linux build completed in ${BUILD_DURATION}s"
        else
            PLATFORM_END=$(date +%s)
            BUILD_DURATION=$((PLATFORM_END - PLATFORM_START))
            BUILD_RESULTS[Linux]="✗ Failed"
            BUILD_TIMES[Linux]=$BUILD_DURATION

            log_error "✗ Linux build failed after ${BUILD_DURATION}s"

            if [ $STOP_ON_ERROR -eq 1 ]; then
                echo ""
                log_error "Stopping build due to Linux failure"
                display_summary
                exit 1
            fi
        fi
    fi

    # ==========================================
    # Windows Build
    # ==========================================
    if [ $BUILD_WINDOWS -eq 1 ]; then
        echo ""
        echo "────────────────────────────────────────────────────────────"
        PLATFORM_NUM=${#PLATFORMS_TO_BUILD[@]}
        log_platform "Building for Windows ($PLATFORM_NUM/${#PLATFORMS_TO_BUILD[@]})"
        echo "────────────────────────────────────────────────────────────"
        echo ""

        PLATFORM_START=$(date +%s)

        # Check if Windows machine is accessible
        if ! ssh -q -o BatchMode=yes -o ConnectTimeout=5 "${WINDOWS_ACT_HOST:-windows-act}" exit 2>/dev/null; then
            log_error "Cannot connect to Windows build machine"
            log_info "Ensure Windows machine is accessible and SSH is configured"

            PLATFORM_END=$(date +%s)
            BUILD_DURATION=$((PLATFORM_END - PLATFORM_START))
            BUILD_RESULTS[Windows]="✗ Failed (No Connection)"
            BUILD_TIMES[Windows]=$BUILD_DURATION

            if [ $STOP_ON_ERROR -eq 1 ]; then
                echo ""
                log_error "Stopping build due to Windows connection failure"
                display_summary
                exit 1
            fi
        else
            log_info "Running Windows build via SSH..."

            if ./scripts/sync-and-build-windows.sh; then
                PLATFORM_END=$(date +%s)
                BUILD_DURATION=$((PLATFORM_END - PLATFORM_START))
                BUILD_RESULTS[Windows]="✓ Success"
                BUILD_TIMES[Windows]=$BUILD_DURATION

                log_info "✓ Windows build completed in ${BUILD_DURATION}s"
            else
                PLATFORM_END=$(date +%s)
                BUILD_DURATION=$((PLATFORM_END - PLATFORM_START))
                BUILD_RESULTS[Windows]="✗ Failed"
                BUILD_TIMES[Windows]=$BUILD_DURATION

                log_error "✗ Windows build failed after ${BUILD_DURATION}s"

                if [ $STOP_ON_ERROR -eq 1 ]; then
                    echo ""
                    log_error "Stopping build due to Windows failure"
                    display_summary
                    exit 1
                fi
            fi
        fi
    fi

    TOTAL_END=$(date +%s)
    TOTAL_DURATION=$((TOTAL_END - TOTAL_START))

    # Display summary
    display_summary
}

# Display build summary
display_summary() {
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  Build Summary"
    echo "══════════════════════════════════════════════════════════════"
    echo ""

    # Display results
    printf "%-15s %-20s %s\n" "Platform" "Status" "Time"
    echo "────────────────────────────────────────────────────────────"

    FAILED_COUNT=0
    SUCCESS_COUNT=0

    for platform in "${!BUILD_RESULTS[@]}"; do
        result="${BUILD_RESULTS[$platform]}"
        time="${BUILD_TIMES[$platform]}"

        printf "%-15s %-20s %ds\n" "$platform" "$result" "$time"

        if [[ $result == *"✗"* ]]; then
            FAILED_COUNT=$((FAILED_COUNT + 1))
        else
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        fi
    done

    echo "────────────────────────────────────────────────────────────"
    echo ""

    if [ -n "$TOTAL_DURATION" ]; then
        log_info "Total build time: ${TOTAL_DURATION}s"
    fi

    echo ""

    if [ $FAILED_COUNT -eq 0 ]; then
        log_info "✓ All platform builds successful!"
        echo ""
        log_info "Next steps:"
        log_info "  - Run tests: ./scripts/test-all-platforms.sh"
        log_info "  - Create packages: make package"
        echo ""
        exit 0
    else
        log_error "✗ $FAILED_COUNT platform build(s) failed, $SUCCESS_COUNT succeeded"
        echo ""
        log_info "To retry individual platforms:"
        log_info "  macOS:   ./scripts/macos-build.sh"
        log_info "  Linux:   act -j build-linux"
        log_info "  Windows: ./scripts/sync-and-build-windows.sh"
        echo ""
        exit 1
    fi
}

main
