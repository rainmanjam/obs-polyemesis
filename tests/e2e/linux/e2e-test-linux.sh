#!/bin/bash
# OBS Polyemesis - Linux End-to-End Test Suite
# Complete plugin lifecycle testing: build → install → load → test → cleanup

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Load test framework
source "$SCRIPT_DIR/../common/test-framework.sh"

# Configuration
OBS_PLUGIN_PATH="${OBS_PLUGIN_PATH:-$HOME/.config/obs-studio/plugins}"
OBS_LOGS_PATH="$HOME/.config/obs-studio/logs"
BUILD_DIR="$PROJECT_ROOT/build"
PLUGIN_NAME="obs-polyemesis.so"

# Test mode
TEST_MODE="${1:-full}"  # full, quick, install-only, ci

# Print header
print_header() {
    echo "========================================"
    echo " OBS Polyemesis - End-to-End Tests"
    echo "========================================"
    echo ""
    echo "Platform: Linux ($(uname -m))"
    echo "Distribution: $(lsb_release -d 2>/dev/null | cut -f2 || echo "Unknown")"
    echo "Test Mode: $TEST_MODE"
    echo "Date: $(date)"
    echo ""
}

# Stage 1: Build Verification
stage_build_verification() {
    log_stage "Build Verification" "1/7"

    if [ "$TEST_MODE" = "quick" ]; then
        log_info "Skipping build in quick mode"
        assert_file_exists "$BUILD_DIR/$PLUGIN_NAME" "Build output exists"
        return 0
    fi

    # Clean previous build
    if [ -d "$BUILD_DIR" ]; then
        log_info "Cleaning previous build..."
        rm -rf "$BUILD_DIR"
    fi

    # Build plugin
    log_info "Building plugin..."
    cd "$PROJECT_ROOT"

    if ! cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_QT=OFF > "$E2E_WORKSPACE/cmake-config.log" 2>&1; then
        log_error "CMake configuration failed"
        cat "$E2E_WORKSPACE/cmake-config.log"
        return 1
    fi

    if ! cmake --build build --config Release > "$E2E_WORKSPACE/cmake-build.log" 2>&1; then
        log_error "Build failed"
        cat "$E2E_WORKSPACE/cmake-build.log"
        return 1
    fi

    # Verify build output
    assert_file_exists "$BUILD_DIR/$PLUGIN_NAME" "Plugin binary created"

    # Check binary type
    local binary_info
    binary_info=$(file "$BUILD_DIR/$PLUGIN_NAME")

    if echo "$binary_info" | grep -q "ELF.*shared object"; then
        log_success "✓ Binary is ELF shared object"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    # Check for architecture
    if echo "$binary_info" | grep -q "x86-64"; then
        log_success "✓ Binary is x86-64"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    elif echo "$binary_info" | grep -q "ARM aarch64"; then
        log_success "✓ Binary is ARM aarch64"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    # Check dependencies
    local deps
    deps=$(ldd "$BUILD_DIR/$PLUGIN_NAME" 2>/dev/null || true)

    if echo "$deps" | grep -q "libobs"; then
        log_success "✓ libobs linked"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    if echo "$deps" | grep -q "libcurl" || nm -D "$BUILD_DIR/$PLUGIN_NAME" 2>/dev/null | grep -q "curl"; then
        log_success "✓ libcurl linked"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi
}

# Stage 2: Installation
stage_installation() {
    log_stage "Installation" "2/7"

    # Create plugin directory
    assert_command_success "mkdir -p \"$OBS_PLUGIN_PATH\"" "Plugin directory created"

    # Remove old installation
    if [ -f "$OBS_PLUGIN_PATH/$PLUGIN_NAME" ]; then
        log_info "Removing previous installation..."
        rm -f "$OBS_PLUGIN_PATH/$PLUGIN_NAME"
    fi

    # Install plugin
    log_info "Installing plugin to OBS..."
    assert_command_success "cp \"$BUILD_DIR/$PLUGIN_NAME\" \"$OBS_PLUGIN_PATH/\"" \
        "Plugin file copied"

    # Verify installation
    assert_file_exists "$OBS_PLUGIN_PATH/$PLUGIN_NAME" "Plugin installed"

    # Check permissions
    if [ -x "$OBS_PLUGIN_PATH/$PLUGIN_NAME" ]; then
        log_success "✓ Binary has execute permissions"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    else
        log_warning "⚠ Binary missing execute permissions, adding..."
        chmod +x "$OBS_PLUGIN_PATH/$PLUGIN_NAME"
    fi
}

# Stage 3: Plugin Loading (via log analysis)
stage_plugin_loading() {
    log_stage "Plugin Loading Verification" "3/7"

    if [ "$TEST_MODE" = "install-only" ]; then
        log_info "Skipping plugin loading in install-only mode"
        return 0
    fi

    # Create logs directory if it doesn't exist
    mkdir -p "$OBS_LOGS_PATH"

    # Check for recent OBS logs
    local latest_log
    latest_log=$(find "$OBS_LOGS_PATH" -name "*.txt" -type f -printf '%T@ %p\n' 2>/dev/null | sort -rn | head -1 | cut -d' ' -f2-)

    if [ -n "$latest_log" ]; then
        log_info "Checking OBS logs: $(basename "$latest_log")"

        # Check if plugin is mentioned in logs
        if grep -q "obs-polyemesis" "$latest_log"; then
            log_success "✓ Plugin mentioned in OBS logs"
            E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
            E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))

            # Check for load errors
            if grep -i "obs-polyemesis" "$latest_log" | grep -i "error" | grep -q .; then
                log_warning "⚠ Potential errors found in logs"
                grep -i "obs-polyemesis" "$latest_log" | grep -i "error" | head -5
            else
                log_success "✓ No plugin errors in logs"
                E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
                E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
            fi

            # Check for module load
            if grep -q "obs_module_load" "$latest_log" && grep -q "polyemesis" "$latest_log"; then
                log_success "✓ Plugin module loaded"
                E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
                E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
            fi
        else
            log_warning "⚠ Plugin not found in logs (OBS may not have been run yet)"
            log_info "  Run OBS Studio manually to generate logs, then re-run tests"
        fi
    else
        log_warning "⚠ No OBS logs found"
        log_info "  Run OBS Studio at least once to generate logs"
    fi
}

# Stage 4: Binary Symbols Verification
stage_binary_symbols() {
    log_stage "Binary Symbols Verification" "4/7"

    local binary="$OBS_PLUGIN_PATH/$PLUGIN_NAME"

    # Check for required OBS symbols
    local symbols
    symbols=$(nm -D "$binary" 2>/dev/null || true)

    if echo "$symbols" | grep -q "obs_module_load"; then
        log_success "✓ obs_module_load symbol exported"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    else
        log_error "✗ obs_module_load symbol missing"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
    fi

    if echo "$symbols" | grep -q "obs_module_unload"; then
        log_success "✓ obs_module_unload symbol exported"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    # Check for jansson (static or dynamic)
    if echo "$symbols" | grep -q "json_"; then
        log_success "✓ JSON symbols present (jansson linked)"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    # Check for curl
    if echo "$symbols" | grep -q "curl_"; then
        log_success "✓ curl symbols present"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi
}

# Stage 5: Library Dependencies
stage_library_dependencies() {
    log_stage "Library Dependencies Verification" "5/7"

    local binary="$OBS_PLUGIN_PATH/$PLUGIN_NAME"

    # Check ldd output
    local ldd_output
    ldd_output=$(ldd "$binary" 2>/dev/null || true)

    # Check for Qt libraries
    if echo "$ldd_output" | grep -q "libQt"; then
        log_success "✓ Qt libraries linked"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    # Check for missing dependencies
    if echo "$ldd_output" | grep -q "not found"; then
        log_error "✗ Missing dependencies detected:"
        echo "$ldd_output" | grep "not found"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
    else
        log_success "✓ All dependencies resolved"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    # Check RPATH/RUNPATH
    local rpath
    rpath=$(readelf -d "$binary" 2>/dev/null | grep -E "RPATH|RUNPATH" || true)
    if [ -n "$rpath" ]; then
        log_verbose "RPATH/RUNPATH: $rpath"
    fi
}

# Stage 6: File Permissions and Security
stage_security_verification() {
    log_stage "Security and Permissions Verification" "6/7"

    local binary="$OBS_PLUGIN_PATH/$PLUGIN_NAME"

    # Check file permissions
    local perms
    perms=$(stat -c "%a" "$binary" 2>/dev/null || stat -f "%Lp" "$binary")
    if [ "$perms" = "755" ] || [ "$perms" = "775" ]; then
        log_success "✓ Correct file permissions: $perms"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    # Check for debugging symbols (should be stripped in release)
    if file "$binary" | grep -q "not stripped"; then
        log_warning "⚠ Binary contains debugging symbols"
    else
        log_success "✓ Binary is stripped (no debug symbols)"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    # Check for security features
    if readelf -W -l "$binary" 2>/dev/null | grep -q "GNU_STACK.*RWE"; then
        log_warning "⚠ Executable stack detected (security risk)"
    elif readelf -W -l "$binary" 2>/dev/null | grep -q "GNU_STACK"; then
        log_success "✓ Non-executable stack (secure)"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi
}

# Stage 7: Cleanup
# shellcheck disable=SC2120
stage_cleanup() {
    log_stage "Cleanup" "7/7"

    # Kill any running OBS instances (if in CI mode)
    if [ "$TEST_MODE" = "ci" ]; then
        kill_process_gracefully "obs" 5
        assert_process_not_running "obs" "OBS process stopped"
    fi

    # Optional: Remove plugin installation
    if [ "$TEST_MODE" = "ci" ] || [ "$1" = "--remove-plugin" ]; then
        log_info "Removing plugin installation..."
        if [ -f "$OBS_PLUGIN_PATH/$PLUGIN_NAME" ]; then
            rm -f "$OBS_PLUGIN_PATH/$PLUGIN_NAME"
            log_success "✓ Plugin uninstalled"
            E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
            E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
        fi
    else
        log_info "Keeping plugin installed for manual testing"
        log_info "  Plugin location: $OBS_PLUGIN_PATH/$PLUGIN_NAME"
        log_info "  To uninstall: rm -f \"$OBS_PLUGIN_PATH/$PLUGIN_NAME\""
    fi

    # Cleanup workspace
    cleanup_workspace
}

# Main test execution
main() {
    print_header
    init_test_framework
    create_workspace

    # Run test stages
    stage_build_verification || true
    stage_installation || true
    stage_plugin_loading || true
    stage_binary_symbols || true
    stage_library_dependencies || true
    stage_security_verification || true
    stage_cleanup || true

    # Print summary
    print_test_summary
    exit_code=$?

    # Instructions for next steps
    if [ $exit_code -eq 0 ]; then
        echo ""
        echo "Next steps:"
        echo "1. Start OBS Studio to verify plugin loads correctly"
        echo "2. Check View menu for 'Restreamer Control' dock"
        echo "3. Add a Restreamer source to verify functionality"
        echo ""
    fi

    exit $exit_code
}

# Run main
main
