#!/bin/bash
# Multi-Platform Testing Helper Script
# Run this from macOS to test across platforms

set -e

COLOR_GREEN='\033[0;32m'
COLOR_RED='\033[0;31m'
COLOR_YELLOW='\033[1;33m'
COLOR_BLUE='\033[0;34m'
NC='\033[0m'

log_info() {
    echo -e "${COLOR_GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${COLOR_RED}[ERROR]${NC} $1"
}

log_warn() {
    echo -e "${COLOR_YELLOW}[WARN]${NC} $1"
}

log_step() {
    echo -e "${COLOR_BLUE}[STEP]${NC} $1"
}

# =============================================================================
# Test on macOS (native)
# =============================================================================
test_macos() {
    log_step "Testing on macOS (native)"

    # Build
    log_info "Building on macOS..."
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

    # Run Python tests
    log_info "Running Python tests..."
    pip3 install -r tests/requirements.txt
    pytest tests/integration/test_obs_integration.py -v

    log_info "✅ macOS tests complete"
}

# =============================================================================
# Test on Linux (Docker)
# =============================================================================
test_linux_docker() {
    log_step "Testing on Linux (Docker)"

    if ! command -v docker &> /dev/null; then
        log_error "Docker not installed. Install from https://docker.com"
        return 1
    fi

    log_info "Building Linux test container..."
    docker build -f Dockerfile.test -t obs-polyemesis:test-linux .

    log_info "Running Linux tests in container..."
    docker run --rm obs-polyemesis:test-linux

    log_info "✅ Linux tests complete"
}

# =============================================================================
# Test on Windows (Parallels VM)
# =============================================================================
test_windows_parallels() {
    log_step "Testing on Windows (Parallels VM)"

    # Check if Parallels is installed
    if [ ! -d "/Applications/Parallels Desktop.app" ]; then
        log_warn "Parallels Desktop not found"
        log_info "Install from: https://www.parallels.com"
        log_info "Or use GitHub Actions for Windows testing"
        return 1
    fi

    # Check if Windows VM is configured
    VM_NAME="${WINDOWS_VM_NAME:-Windows 11}"

    log_info "Looking for Windows VM: $VM_NAME"

    if ! prlctl list -a | grep -q "$VM_NAME"; then
        log_warn "Windows VM '$VM_NAME' not found"
        log_info "Set WINDOWS_VM_NAME environment variable or create a VM"
        log_info "Alternative: Use GitHub Actions for Windows testing"
        return 1
    fi

    # Start VM if not running
    if ! prlctl list | grep -q "$VM_NAME"; then
        log_info "Starting Windows VM..."
        prlctl start "$VM_NAME"
        sleep 30
    fi

    # Create test script for Windows
    cat > /tmp/windows-test.ps1 << 'EOF'
# Windows build and test script
Write-Host "Building on Windows..." -ForegroundColor Green

# Navigate to shared folder
cd "C:\Users\parallels\Desktop\obs-polyemesis"

# Build
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run tests
pip install -r tests\requirements.txt
pytest tests\integration\test_obs_integration.py -v

Write-Host "Windows tests complete" -ForegroundColor Green
EOF

    log_info "Running Windows tests..."
    log_warn "You need to:"
    log_warn "1. Share your project folder with Parallels VM"
    log_warn "2. Install: Visual Studio, CMake, Python, pytest"
    log_warn "3. Run: powershell -File C:\\path\\to\\windows-test.ps1"
    log_info ""
    log_info "Test script created at: /tmp/windows-test.ps1"
    log_info "Copy this to your Windows VM and run it"
}

# =============================================================================
# Test with GitHub Actions locally (act)
# =============================================================================
test_with_act() {
    log_step "Testing with GitHub Actions (act)"

    if ! command -v act &> /dev/null; then
        log_warn "act not installed"
        log_info "Install with: brew install act"
        return 1
    fi

    log_info "Testing Linux workflow with act..."
    act -W .github/workflows/test.yml -j build-and-test \
        --container-architecture linux/amd64 \
        --matrix os:ubuntu-latest

    log_info "✅ act tests complete (Linux only)"
    log_warn "Note: act only supports Linux. Use Parallels or GitHub Actions for Windows/macOS"
}

# =============================================================================
# Platform-specific build validation
# =============================================================================
validate_cross_platform_code() {
    log_step "Validating cross-platform code patterns"

    log_info "Checking for platform-specific issues..."

    # Check for hardcoded paths
    if grep -r "C:\\\\" src/ 2>/dev/null; then
        log_warn "Found Windows hardcoded paths in src/"
    fi

    if grep -r "/usr/local" src/ 2>/dev/null; then
        log_warn "Found Unix hardcoded paths in src/"
    fi

    # Check for platform-specific includes
    log_info "Checking platform guards..."
    if ! grep -r "#ifdef _WIN32" src/ > /dev/null 2>&1; then
        log_warn "No Windows platform guards found - may need platform-specific code"
    fi

    log_info "✅ Code validation complete"
}

# =============================================================================
# Main menu
# =============================================================================
show_menu() {
    echo ""
    echo "========================================"
    echo "   Multi-Platform Testing Menu"
    echo "========================================"
    echo ""
    echo "1. Test on macOS (native)"
    echo "2. Test on Linux (Docker)"
    echo "3. Test on Windows (Parallels)"
    echo "4. Test with act (GitHub Actions locally)"
    echo "5. Validate cross-platform code"
    echo "6. Run ALL local tests"
    echo "7. Setup instructions"
    echo "0. Exit"
    echo ""
}

show_setup_instructions() {
    cat << 'EOF'

======================================
    Platform Setup Instructions
======================================

macOS (Current Platform):
  ✓ Already configured
  - Ensure you have: CMake, Python, pip
  - Run: pip3 install -r tests/requirements.txt

Linux (Docker):
  1. Install Docker Desktop for Mac
  2. Docker will build and test automatically
  3. No additional setup needed!

Windows (Parallels Desktop):
  1. Install Parallels Desktop (https://www.parallels.com)
  2. Create Windows 11 VM (or use existing)
  3. In Windows VM, install:
     - Visual Studio 2022 Build Tools
     - CMake (choco install cmake)
     - Python 3.11 (choco install python)
     - Git (choco install git)
  4. Share your macOS project folder with VM
  5. Map as: C:\Users\parallels\Desktop\obs-polyemesis

GitHub Actions (Recommended for Windows):
  - Simply push your code
  - CI runs on real Windows, macOS, Linux
  - No local setup needed!
  - View results at:
    https://github.com/rainmanjam/obs-polyemesis/actions

======================================
EOF
}

# =============================================================================
# Main
# =============================================================================
main() {
    while true; do
        show_menu
        read -p "Select option: " choice

        case $choice in
            1) test_macos ;;
            2) test_linux_docker ;;
            3) test_windows_parallels ;;
            4) test_with_act ;;
            5) validate_cross_platform_code ;;
            6)
                test_macos
                test_linux_docker
                validate_cross_platform_code
                log_info "For Windows testing, use option 3 or GitHub Actions"
                ;;
            7) show_setup_instructions ;;
            0)
                log_info "Goodbye!"
                exit 0
                ;;
            *)
                log_error "Invalid option"
                ;;
        esac

        echo ""
        read -p "Press Enter to continue..."
    done
}

# Run main menu if no arguments
if [ $# -eq 0 ]; then
    main
else
    # Allow running specific tests from command line
    case $1 in
        macos) test_macos ;;
        linux) test_linux_docker ;;
        windows) test_windows_parallels ;;
        act) test_with_act ;;
        validate) validate_cross_platform_code ;;
        *)
            log_error "Usage: $0 [macos|linux|windows|act|validate]"
            exit 1
            ;;
    esac
fi
