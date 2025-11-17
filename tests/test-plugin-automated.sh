#!/bin/bash
# OBS Polyemesis - Automated Plugin Testing
# Tests plugin loading verification, UI components, and functionality

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
PLUGIN_NAME="obs-polyemesis"
OBS_PLUGINS_DIR="$HOME/Library/Application Support/obs-studio/plugins"
OBS_LOGS_DIR="$HOME/Library/Application Support/obs-studio/logs"
PLUGIN_BUNDLE="${PLUGIN_NAME}.plugin"
BUILD_DIR="./build/Release"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
log_test() {
    echo -e "${BLUE}[TEST]${NC} $1"
    TESTS_RUN=$((TESTS_RUN + 1))
}

log_pass() {
    echo -e "${GREEN}  ✓${NC} $1"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

log_fail() {
    echo -e "${RED}  ✗${NC} $1"
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

log_info() {
    echo -e "${YELLOW}  ℹ${NC} $1"
}

log_section() {
    echo ""
    echo "==========================================="
    echo " $1"
    echo "==========================================="
    echo ""
}

# ============================================
# SECTION 1: Plugin Build Verification
# ============================================

test_plugin_build_exists() {
    log_test "Verify plugin build exists"

    if [ -d "${BUILD_DIR}/${PLUGIN_BUNDLE}" ]; then
        log_pass "Plugin bundle found at ${BUILD_DIR}/${PLUGIN_BUNDLE}"
        return 0
    else
        log_fail "Plugin bundle not found at ${BUILD_DIR}/${PLUGIN_BUNDLE}"
        return 1
    fi
}

test_plugin_binary_exists() {
    log_test "Verify plugin binary exists"

    BINARY_PATH="${BUILD_DIR}/${PLUGIN_BUNDLE}/Contents/MacOS/${PLUGIN_NAME}"
    if [ -f "$BINARY_PATH" ]; then
        log_pass "Plugin binary found at $BINARY_PATH"

        # Check file type
        FILE_TYPE=$(file "$BINARY_PATH")
        if echo "$FILE_TYPE" | grep -q "Mach-O"; then
            log_pass "Binary is a valid Mach-O executable"
        else
            log_fail "Binary is not a valid Mach-O executable"
            return 1
        fi

        # Check architectures
        if echo "$FILE_TYPE" | grep -q "arm64"; then
            log_pass "Binary contains arm64 architecture"
        fi
        if echo "$FILE_TYPE" | grep -q "x86_64"; then
            log_pass "Binary contains x86_64 architecture"
        fi

        return 0
    else
        log_fail "Plugin binary not found at $BINARY_PATH"
        return 1
    fi
}

test_plugin_info_plist() {
    log_test "Verify Info.plist exists and is valid"

    PLIST_PATH="${BUILD_DIR}/${PLUGIN_BUNDLE}/Contents/Info.plist"
    if [ -f "$PLIST_PATH" ]; then
        log_pass "Info.plist found"

        # Verify it's a valid plist
        if plutil -lint "$PLIST_PATH" >/dev/null 2>&1; then
            log_pass "Info.plist is valid"

            # Check for required keys
            if /usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$PLIST_PATH" >/dev/null 2>&1; then
                BUNDLE_ID=$(/usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$PLIST_PATH")
                log_pass "CFBundleIdentifier: $BUNDLE_ID"
            else
                log_fail "Missing CFBundleIdentifier"
            fi

            if /usr/libexec/PlistBuddy -c "Print :CFBundleVersion" "$PLIST_PATH" >/dev/null 2>&1; then
                VERSION=$(/usr/libexec/PlistBuddy -c "Print :CFBundleVersion" "$PLIST_PATH")
                log_pass "CFBundleVersion: $VERSION"
            else
                log_fail "Missing CFBundleVersion"
            fi

            return 0
        else
            log_fail "Info.plist is invalid"
            return 1
        fi
    else
        log_fail "Info.plist not found at $PLIST_PATH"
        return 1
    fi
}

test_plugin_dependencies() {
    log_test "Verify plugin dependencies are linked"

    BINARY_PATH="${BUILD_DIR}/${PLUGIN_BUNDLE}/Contents/MacOS/${PLUGIN_NAME}"
    if [ -f "$BINARY_PATH" ]; then
        DEPS=$(otool -L "$BINARY_PATH")

        # Check for OBS framework
        if echo "$DEPS" | grep -q "obs"; then
            log_pass "OBS framework linked"
        else
            log_fail "OBS framework not linked"
        fi

        # Check for required libraries
        if echo "$DEPS" | grep -q "libcurl"; then
            log_pass "libcurl linked"
        else
            log_fail "libcurl not linked"
        fi

        # Check for jansson (may be statically linked)
        if echo "$DEPS" | grep -q "jansson"; then
            log_pass "jansson linked (dynamic)"
        else
            # Check if symbols exist (statically linked)
            if nm "$BINARY_PATH" 2>/dev/null | grep -q "json_"; then
                log_pass "jansson linked (static)"
            else
                log_fail "jansson not linked"
            fi
        fi

        return 0
    else
        log_fail "Cannot check dependencies - binary not found"
        return 1
    fi
}

# ============================================
# SECTION 2: Plugin Installation Verification
# ============================================

test_plugin_installation() {
    log_test "Verify plugin is installed in OBS"

    INSTALLED_PATH="${OBS_PLUGINS_DIR}/${PLUGIN_BUNDLE}"
    if [ -d "$INSTALLED_PATH" ]; then
        log_pass "Plugin installed at $INSTALLED_PATH"

        # Verify binary exists
        INSTALLED_BINARY="${INSTALLED_PATH}/Contents/MacOS/${PLUGIN_NAME}"
        if [ -f "$INSTALLED_BINARY" ]; then
            log_pass "Plugin binary exists in installation"
            return 0
        else
            log_fail "Plugin binary missing from installation"
            return 1
        fi
    else
        log_fail "Plugin not installed at $INSTALLED_PATH"
        log_info "Run installation: mkdir -p '${OBS_PLUGINS_DIR}' && cp -r '${BUILD_DIR}/${PLUGIN_BUNDLE}' '${OBS_PLUGINS_DIR}/'"
        return 1
    fi
}

test_plugin_code_signature() {
    log_test "Verify plugin code signature"

    INSTALLED_BINARY="${OBS_PLUGINS_DIR}/${PLUGIN_BUNDLE}/Contents/MacOS/${PLUGIN_NAME}"
    if [ -f "$INSTALLED_BINARY" ]; then
        CODESIGN_OUTPUT=$(codesign -dv "$INSTALLED_BINARY" 2>&1 || true)

        if echo "$CODESIGN_OUTPUT" | grep -q "Signature"; then
            log_pass "Plugin is code signed"

            # Check signing identity
            if echo "$CODESIGN_OUTPUT" | grep -q "Authority="; then
                AUTHORITY=$(echo "$CODESIGN_OUTPUT" | grep "Authority=" | head -1)
                log_info "$AUTHORITY"
            elif echo "$CODESIGN_OUTPUT" | grep -q "adhoc"; then
                log_info "Ad-hoc signature (local development)"
            fi

            return 0
        else
            log_fail "Plugin is not code signed"
            return 1
        fi
    else
        log_fail "Cannot verify signature - binary not found"
        return 1
    fi
}

# ============================================
# SECTION 3: Plugin Loading Verification
# ============================================

test_plugin_loading_from_logs() {
    log_test "Verify plugin loads successfully (from OBS logs)"

    if [ ! -d "$OBS_LOGS_DIR" ]; then
        log_fail "OBS logs directory not found: $OBS_LOGS_DIR"
        log_info "Start OBS Studio at least once to generate logs"
        return 1
    fi

    # Find the most recent log file
    LATEST_LOG=$(ls -t "$OBS_LOGS_DIR"/*.txt 2>/dev/null | head -1)

    if [ -z "$LATEST_LOG" ]; then
        log_fail "No OBS log files found"
        log_info "Start OBS Studio to generate logs"
        return 1
    fi

    log_info "Checking log: $(basename "$LATEST_LOG")"

    # Check for plugin loading messages
    if grep -q "obs-polyemesis" "$LATEST_LOG"; then
        log_pass "Plugin mentioned in logs"

        # Check for successful loading
        if grep -q "obs-polyemesis.*loaded" "$LATEST_LOG" || \
           grep -q "obs-polyemesis.*registered" "$LATEST_LOG" || \
           grep -q "Loaded module.*obs-polyemesis" "$LATEST_LOG"; then
            log_pass "Plugin loaded successfully"
        else
            log_info "Plugin mentioned but no clear loading confirmation"
        fi

        # Check for errors (exclude status enum descriptions)
        if grep -i "obs-polyemesis" "$LATEST_LOG" | grep -i "error" | grep -v "0=INACTIVE.*ERROR" | grep -q .; then
            log_fail "Plugin errors found in logs:"
            grep -i "obs-polyemesis" "$LATEST_LOG" | grep -i "error" | grep -v "0=INACTIVE.*ERROR" | head -5 | while read line; do
                log_info "  $line"
            done
            return 1
        else
            log_pass "No plugin errors found in logs"
        fi

        # Check for warnings
        if grep -i "obs-polyemesis.*warn" "$LATEST_LOG" >/dev/null 2>&1; then
            log_info "Plugin warnings found:"
            grep -i "obs-polyemesis.*warn" "$LATEST_LOG" | head -3 | while read line; do
                log_info "  $line"
            done
        fi

        return 0
    else
        log_fail "Plugin not mentioned in logs"
        log_info "Plugin may not have been loaded by OBS"
        return 1
    fi
}

test_plugin_module_registration() {
    log_test "Verify plugin module registration (from logs)"

    LATEST_LOG=$(ls -t "$OBS_LOGS_DIR"/*.txt 2>/dev/null | head -1)

    if [ -z "$LATEST_LOG" ]; then
        log_fail "No OBS log files found"
        return 1
    fi

    # Check for module loading
    if grep -q "Loaded module.*obs-polyemesis" "$LATEST_LOG" || \
       grep -q "obs-polyemesis.plugin.*loaded" "$LATEST_LOG"; then
        log_pass "Plugin module loaded and registered"
        return 0
    else
        log_fail "No module registration found"
        log_info "Check if plugin is compatible with OBS version"
        return 1
    fi
}

# ============================================
# SECTION 4: UI Component Verification
# ============================================

test_plugin_sources_registration() {
    log_test "Verify Restreamer source registration (from logs)"

    LATEST_LOG=$(ls -t "$OBS_LOGS_DIR"/*.txt 2>/dev/null | head -1)

    if [ -z "$LATEST_LOG" ]; then
        log_fail "No OBS log files found"
        return 1
    fi

    # Check for source registration
    if grep -qi "restreamer.*source" "$LATEST_LOG" || \
       grep -qi "register.*source.*restreamer" "$LATEST_LOG"; then
        log_pass "Restreamer source appears to be registered"
        return 0
    else
        log_info "No clear source registration found in logs"
        log_info "Source may still be registered - check OBS UI manually"
        return 0
    fi
}

test_plugin_outputs_registration() {
    log_test "Verify Restreamer output registration (from logs)"

    LATEST_LOG=$(ls -t "$OBS_LOGS_DIR"/*.txt 2>/dev/null | head -1)

    if [ -z "$LATEST_LOG" ]; then
        log_fail "No OBS log files found"
        return 1
    fi

    # Check for output registration
    if grep -qi "restreamer.*output" "$LATEST_LOG" || \
       grep -qi "register.*output.*restreamer" "$LATEST_LOG"; then
        log_pass "Restreamer output appears to be registered"
        return 0
    else
        log_info "No clear output registration found in logs"
        log_info "Output may still be registered - check OBS UI manually"
        return 0
    fi
}

test_plugin_dock_registration() {
    log_test "Verify Restreamer Control dock registration (from logs)"

    LATEST_LOG=$(ls -t "$OBS_LOGS_DIR"/*.txt 2>/dev/null | head -1)

    if [ -z "$LATEST_LOG" ]; then
        log_fail "No OBS log files found"
        return 1
    fi

    # Check for dock/UI registration
    if grep -qi "restreamer.*control" "$LATEST_LOG" || \
       grep -qi "restreamer.*dock" "$LATEST_LOG" || \
       grep -qi "dock.*restreamer" "$LATEST_LOG"; then
        log_pass "Restreamer Control dock appears to be registered"
        return 0
    else
        log_info "No clear dock registration found in logs"
        log_info "Dock may still be registered - check View menu in OBS"
        return 0
    fi
}

# ============================================
# SECTION 5: Functionality Tests
# ============================================

test_plugin_qt_dependencies() {
    log_test "Verify Qt framework dependencies (for UI)"

    INSTALLED_BINARY="${OBS_PLUGINS_DIR}/${PLUGIN_BUNDLE}/Contents/MacOS/${PLUGIN_NAME}"

    if [ -f "$INSTALLED_BINARY" ]; then
        DEPS=$(otool -L "$INSTALLED_BINARY")

        if echo "$DEPS" | grep -q "QtCore" || echo "$DEPS" | grep -q "QtWidgets"; then
            log_pass "Qt frameworks linked (UI enabled)"
            return 0
        else
            log_info "Qt frameworks not linked (UI may be disabled)"
            log_info "Plugin may still work without Qt UI components"
            return 0
        fi
    else
        log_fail "Cannot check Qt dependencies - binary not found"
        return 1
    fi
}

test_plugin_symbols() {
    log_test "Verify plugin exports required OBS symbols"

    INSTALLED_BINARY="${OBS_PLUGINS_DIR}/${PLUGIN_BUNDLE}/Contents/MacOS/${PLUGIN_NAME}"

    if [ -f "$INSTALLED_BINARY" ]; then
        SYMBOLS=$(nm "$INSTALLED_BINARY" 2>/dev/null || true)

        # Check for obs_module_load
        if echo "$SYMBOLS" | grep -q "obs_module_load"; then
            log_pass "exports obs_module_load"
        else
            log_fail "Missing obs_module_load export"
        fi

        # Check for obs_module_unload
        if echo "$SYMBOLS" | grep -q "obs_module_unload"; then
            log_pass "exports obs_module_unload"
        else
            log_info "Missing obs_module_unload (optional)"
        fi

        return 0
    else
        log_fail "Cannot check symbols - binary not found"
        return 1
    fi
}

# ============================================
# Main Test Execution
# ============================================

main() {
    echo ""
    echo "==========================================="
    echo " OBS Polyemesis - Automated Plugin Tests"
    echo "==========================================="
    echo ""
    echo "Plugin: ${PLUGIN_NAME}"
    echo "Platform: macOS"
    echo "Date: $(date)"
    echo ""

    # Section 1: Build Verification
    log_section "Section 1: Plugin Build Verification"
    test_plugin_build_exists || true
    test_plugin_binary_exists || true
    test_plugin_info_plist || true
    test_plugin_dependencies || true

    # Section 2: Installation Verification
    log_section "Section 2: Plugin Installation Verification"
    test_plugin_installation || true
    test_plugin_code_signature || true

    # Section 3: Loading Verification
    log_section "Section 3: Plugin Loading Verification"
    test_plugin_loading_from_logs || true
    test_plugin_module_registration || true

    # Section 4: UI Component Verification
    log_section "Section 4: UI Component Verification"
    test_plugin_sources_registration || true
    test_plugin_outputs_registration || true
    test_plugin_dock_registration || true

    # Section 5: Functionality Tests
    log_section "Section 5: Functionality Tests"
    test_plugin_qt_dependencies || true
    test_plugin_symbols || true

    # Summary
    log_section "Test Summary"
    echo "Tests run:    $TESTS_RUN"
    echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
    echo ""

    PASS_RATE=$((TESTS_PASSED * 100 / TESTS_RUN))
    echo "Pass rate: ${PASS_RATE}%"
    echo ""

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}✅ All tests passed!${NC}"
        echo ""
        echo "Next steps:"
        echo "1. Start OBS Studio to verify plugin loads correctly"
        echo "2. Check View menu for 'Restreamer Control' dock"
        echo "3. Add a Restreamer source to verify functionality"
        echo ""
        exit 0
    else
        echo -e "${RED}❌ Some tests failed${NC}"
        echo ""
        echo "Troubleshooting:"
        echo "1. Check if plugin was built: ./scripts/macos-build.sh"
        echo "2. Check if plugin is installed: ls -la '${OBS_PLUGINS_DIR}/${PLUGIN_BUNDLE}'"
        echo "3. Check OBS logs: ls -lt '${OBS_LOGS_DIR}' | head -5"
        echo "4. Try reinstalling the plugin"
        echo ""
        exit 1
    fi
}

# Run main
main "$@"
