#!/bin/bash
# OBS Polyemesis - macOS End-to-End Test Suite
# Complete plugin lifecycle testing: build → install → load → test → cleanup

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Load test framework
source "$SCRIPT_DIR/../common/test-framework.sh"

# Configuration
OBS_PLUGIN_PATH="${OBS_PLUGIN_PATH:-$HOME/Library/Application Support/obs-studio/plugins}"
OBS_LOGS_PATH="$HOME/Library/Application Support/obs-studio/logs"
BUILD_DIR="$PROJECT_ROOT/build"
PLUGIN_BUNDLE="obs-polyemesis.plugin"
PLUGIN_NAME="obs-polyemesis"

# Test mode
TEST_MODE="${1:-full}"  # full, quick, install-only, ci

# Print header
print_header() {
    echo "========================================"
    echo " OBS Polyemesis - End-to-End Tests"
    echo "========================================"
    echo ""
    echo "Platform: macOS ($(uname -m))"
    echo "Test Mode: $TEST_MODE"
    echo "Date: $(date)"
    echo ""
}

# Stage 1: Build Verification
stage_build_verification() {
    log_stage "Build Verification" "1/7"

    if [ "$TEST_MODE" = "quick" ]; then
        log_info "Skipping build in quick mode"
        assert_dir_exists "$BUILD_DIR/Release/$PLUGIN_BUNDLE" "Build directory exists"
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

    if ! cmake -S . -B build -G Xcode \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_SCRIPTING=OFF \
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
    assert_dir_exists "$BUILD_DIR/Release/$PLUGIN_BUNDLE" "Plugin bundle created"
    assert_file_exists "$BUILD_DIR/Release/$PLUGIN_BUNDLE/Contents/MacOS/$PLUGIN_NAME" "Plugin binary exists"

    # Check binary architecture
    local binary="$BUILD_DIR/Release/$PLUGIN_BUNDLE/Contents/MacOS/$PLUGIN_NAME"
    local arch_info
    arch_info=$(file "$binary")

    if echo "$arch_info" | grep -q "arm64"; then
        log_success "✓ Binary contains arm64 architecture"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    if echo "$arch_info" | grep -q "x86_64"; then
        log_success "✓ Binary contains x86_64 architecture"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    # Check dependencies
    local deps
    deps=$(otool -L "$binary")

    if echo "$deps" | grep -q "obs"; then
        log_success "✓ OBS framework linked"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    if echo "$deps" | grep -q "curl" || nm "$binary" 2>/dev/null | grep -q "curl"; then
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
    if [ -d "${OBS_PLUGIN_PATH:?}/$PLUGIN_BUNDLE" ]; then
        log_info "Removing previous installation..."
        rm -rf "${OBS_PLUGIN_PATH:?}/$PLUGIN_BUNDLE"
    fi

    # Install plugin
    log_info "Installing plugin to OBS..."
    assert_command_success "cp -r \"$BUILD_DIR/Release/$PLUGIN_BUNDLE\" \"$OBS_PLUGIN_PATH/\"" \
        "Plugin files copied"

    # Verify installation
    assert_dir_exists "$OBS_PLUGIN_PATH/$PLUGIN_BUNDLE" "Plugin installed"
    assert_file_exists "$OBS_PLUGIN_PATH/$PLUGIN_BUNDLE/Contents/MacOS/$PLUGIN_NAME" \
        "Plugin binary installed"

    # Check permissions
    if [ -x "$OBS_PLUGIN_PATH/$PLUGIN_BUNDLE/Contents/MacOS/$PLUGIN_NAME" ]; then
        log_success "✓ Binary has execute permissions"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    else
        log_error "✗ Binary missing execute permissions"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
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
    latest_log=$(find "$OBS_LOGS_PATH" -name "*.txt" -type f -print0 2>/dev/null | xargs -0 ls -t 2>/dev/null | head -1)

    if [ -n "$latest_log" ]; then
        log_info "Checking OBS logs: $(basename "$latest_log")"

        # Check if plugin is mentioned in logs
        if grep -q "$PLUGIN_NAME" "$latest_log"; then
            log_success "✓ Plugin mentioned in OBS logs"
            E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
            E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))

            # Check for load errors
            if grep -i "$PLUGIN_NAME" "$latest_log" | grep -i "error" | grep -v "0=INACTIVE.*ERROR" | grep -q .; then
                log_warning "⚠ Potential errors found in logs"
                grep -i "$PLUGIN_NAME" "$latest_log" | grep -i "error" | grep -v "0=INACTIVE.*ERROR" | head -5
            else
                log_success "✓ No plugin errors in logs"
                E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
                E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
            fi

            # Check for module load
            if grep -q "obs_module_load.*$PLUGIN_NAME" "$latest_log" || \
               grep -q "Loaded module.*$PLUGIN_NAME" "$latest_log"; then
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

    local binary="$OBS_PLUGIN_PATH/$PLUGIN_BUNDLE/Contents/MacOS/$PLUGIN_NAME"

    # Check for required OBS symbols
    local symbols
    symbols=$(nm "$binary" 2>/dev/null || true)

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

# Stage 5: Info.plist Verification
stage_plist_verification() {
    log_stage "Info.plist Verification" "5/7"

    local plist="$OBS_PLUGIN_PATH/$PLUGIN_BUNDLE/Contents/Info.plist"

    assert_file_exists "$plist" "Info.plist exists"

    # Check for required keys
    if /usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$plist" >/dev/null 2>&1; then
        local bundle_id
        bundle_id=$(/usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$plist")
        log_success "✓ CFBundleIdentifier: $bundle_id"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    if /usr/libexec/PlistBuddy -c "Print :CFBundleVersion" "$plist" >/dev/null 2>&1; then
        local version
        version=$(/usr/libexec/PlistBuddy -c "Print :CFBundleVersion" "$plist")
        log_success "✓ CFBundleVersion: $version"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi

    if /usr/libexec/PlistBuddy -c "Print :CFBundleName" "$plist" >/dev/null 2>&1; then
        local name
        name=$(/usr/libexec/PlistBuddy -c "Print :CFBundleName" "$plist")
        log_success "✓ CFBundleName: $name"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    fi
}

# Stage 6: Code Signing Verification
stage_code_signing() {
    log_stage "Code Signing Verification" "6/7"

    local binary="$OBS_PLUGIN_PATH/$PLUGIN_BUNDLE/Contents/MacOS/$PLUGIN_NAME"

    # Check code signing
    if codesign -v "$binary" 2>/dev/null; then
        log_success "✓ Binary is code signed"
        E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
    else
        log_warning "⚠ Binary not code signed (expected for local builds)"
    fi

    # Check ad-hoc signing
    local sign_info
    sign_info=$(codesign -dv "$binary" 2>&1 || true)
    if echo "$sign_info" | grep -q "adhoc"; then
        log_success "✓ Ad-hoc code signature present"
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
        if [ -d "${OBS_PLUGIN_PATH:?}/$PLUGIN_BUNDLE" ]; then
            rm -rf "${OBS_PLUGIN_PATH:?}/$PLUGIN_BUNDLE"
            log_success "✓ Plugin uninstalled"
            E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))
            E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
        fi
    else
        log_info "Keeping plugin installed for manual testing"
        log_info "  Plugin location: $OBS_PLUGIN_PATH/$PLUGIN_BUNDLE"
        log_info "  To uninstall: rm -rf \"$OBS_PLUGIN_PATH/$PLUGIN_BUNDLE\""
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
    stage_plist_verification || true
    stage_code_signing || true
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
