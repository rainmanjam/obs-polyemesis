#!/bin/bash
# OBS Polyemesis - E2E Test Framework
# Common utilities and functions for end-to-end testing

set -e

# Colors
export RED='\033[0;31m'
export GREEN='\033[0;32m'
export YELLOW='\033[1;33m'
export BLUE='\033[0;34m'
export CYAN='\033[0;36m'
export NC='\033[0m'

# Test state
export E2E_TESTS_RUN=0
export E2E_TESTS_PASSED=0
export E2E_TESTS_FAILED=0
export E2E_START_TIME
export E2E_CURRENT_STAGE=""

# Configuration
export E2E_VERBOSITY=${E2E_VERBOSITY:-1}
export E2E_KEEP_ON_FAILURE=${E2E_KEEP_ON_FAILURE:-true}
export E2E_TIMEOUT=${E2E_TIMEOUT:-60}
export E2E_WORKSPACE=${E2E_WORKSPACE:-/tmp/obs-polyemesis-e2e}

# Logging functions
log_info() {
    if [ "$E2E_VERBOSITY" -ge 1 ]; then
        echo -e "${BLUE}[INFO]${NC} $1"
    fi
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_verbose() {
    if [ "$E2E_VERBOSITY" -ge 2 ]; then
        echo -e "${CYAN}[DEBUG]${NC} $1"
    fi
}

log_stage() {
    E2E_CURRENT_STAGE="$1"
    echo ""
    echo -e "${CYAN}[STAGE $2] $1${NC}"
    echo "----------------------------------------"
}

# Test assertion functions
assert_file_exists() {
    local file="$1"
    local desc="${2:-File exists: $file}"

    E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))

    if [ -f "$file" ]; then
        log_success "✓ $desc"
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
        return 0
    else
        log_error "✗ $desc"
        log_error "  File not found: $file"
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
        return 1
    fi
}

assert_dir_exists() {
    local dir="$1"
    local desc="${2:-Directory exists: $dir}"

    E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))

    if [ -d "$dir" ]; then
        log_success "✓ $desc"
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
        return 0
    else
        log_error "✗ $desc"
        log_error "  Directory not found: $dir"
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
        return 1
    fi
}

assert_command_success() {
    local cmd="$1"
    local desc="${2:-Command succeeded: $cmd}"

    E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))

    log_verbose "Running: $cmd"

    if eval "$cmd" >/dev/null 2>&1; then
        log_success "✓ $desc"
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
        return 0
    else
        log_error "✗ $desc"
        log_error "  Command failed: $cmd"
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
        return 1
    fi
}

assert_string_contains() {
    local haystack="$1"
    local needle="$2"
    local desc="${3:-String contains: $needle}"

    E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))

    if echo "$haystack" | grep -q "$needle"; then
        log_success "✓ $desc"
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
        return 0
    else
        log_error "✗ $desc"
        log_error "  String not found: $needle"
        log_verbose "  In: $haystack"
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
        return 1
    fi
}

assert_file_contains() {
    local file="$1"
    local pattern="$2"
    local desc="${3:-File contains pattern: $pattern}"

    E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))

    if [ ! -f "$file" ]; then
        log_error "✗ $desc"
        log_error "  File not found: $file"
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
        return 1
    fi

    if grep -q "$pattern" "$file"; then
        log_success "✓ $desc"
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
        return 0
    else
        log_error "✗ $desc"
        log_error "  Pattern not found: $pattern"
        log_error "  In file: $file"
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
        return 1
    fi
}

assert_not_file_contains() {
    local file="$1"
    local pattern="$2"
    local desc="${3:-File does not contain pattern: $pattern}"

    E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))

    if [ ! -f "$file" ]; then
        log_error "✗ $desc"
        log_error "  File not found: $file"
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
        return 1
    fi

    if ! grep -q "$pattern" "$file"; then
        log_success "✓ $desc"
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
        return 0
    else
        log_error "✗ $desc"
        log_error "  Pattern found (should not be): $pattern"
        log_error "  In file: $file"
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
        return 1
    fi
}

assert_process_running() {
    local process_name="$1"
    local desc="${2:-Process is running: $process_name}"

    E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))

    if pgrep -x "$process_name" >/dev/null 2>&1; then
        log_success "✓ $desc"
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
        return 0
    else
        log_error "✗ $desc"
        log_error "  Process not running: $process_name"
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
        return 1
    fi
}

assert_process_not_running() {
    local process_name="$1"
    local desc="${2:-Process is not running: $process_name}"

    E2E_TESTS_RUN=$((E2E_TESTS_RUN + 1))

    if ! pgrep -x "$process_name" >/dev/null 2>&1; then
        log_success "✓ $desc"
        E2E_TESTS_PASSED=$((E2E_TESTS_PASSED + 1))
        return 0
    else
        log_error "✗ $desc"
        log_error "  Process is running: $process_name"
        E2E_TESTS_FAILED=$((E2E_TESTS_FAILED + 1))
        return 1
    fi
}

# Utility functions
create_workspace() {
    log_info "Creating test workspace: $E2E_WORKSPACE"
    mkdir -p "$E2E_WORKSPACE"
    log_verbose "Workspace created"
}

cleanup_workspace() {
    if [ "$E2E_KEEP_ON_FAILURE" = "true" ] && [ "$E2E_TESTS_FAILED" -gt 0 ]; then
        log_warning "Keeping workspace due to test failures: $E2E_WORKSPACE"
        return 0
    fi

    if [ -d "$E2E_WORKSPACE" ]; then
        log_info "Cleaning up workspace: $E2E_WORKSPACE"
        rm -rf "$E2E_WORKSPACE"
        log_verbose "Workspace cleaned"
    fi
}

wait_for_process() {
    local process_name="$1"
    local timeout="${2:-$E2E_TIMEOUT}"
    local count=0

    log_verbose "Waiting for process: $process_name (timeout: ${timeout}s)"

    while [ $count -lt "$timeout" ]; do
        if pgrep -x "$process_name" >/dev/null 2>&1; then
            log_verbose "Process found: $process_name"
            return 0
        fi
        sleep 1
        count=$((count + 1))
    done

    log_error "Timeout waiting for process: $process_name"
    return 1
}

wait_for_file() {
    local file="$1"
    local timeout="${2:-$E2E_TIMEOUT}"
    local count=0

    log_verbose "Waiting for file: $file (timeout: ${timeout}s)"

    while [ $count -lt "$timeout" ]; do
        if [ -f "$file" ]; then
            log_verbose "File found: $file"
            return 0
        fi
        sleep 1
        count=$((count + 1))
    done

    log_error "Timeout waiting for file: $file"
    return 1
}

kill_process_gracefully() {
    local process_name="$1"
    local timeout="${2:-10}"

    if ! pgrep -x "$process_name" >/dev/null 2>&1; then
        log_verbose "Process not running: $process_name"
        return 0
    fi

    log_verbose "Stopping process: $process_name"
    pkill -TERM "$process_name" 2>/dev/null || true

    local count=0
    while [ $count -lt "$timeout" ]; do
        if ! pgrep -x "$process_name" >/dev/null 2>&1; then
            log_verbose "Process stopped: $process_name"
            return 0
        fi
        sleep 1
        count=$((count + 1))
    done

    log_warning "Force killing process: $process_name"
    pkill -KILL "$process_name" 2>/dev/null || true
    sleep 1
}

# Test summary and reporting
print_test_summary() {
    local duration=$(($(date +%s) - E2E_START_TIME))

    echo ""
    echo "========================================"
    echo " Test Summary"
    echo "========================================"
    echo ""
    echo "Total Tests: $E2E_TESTS_RUN"
    echo "Passed:      $E2E_TESTS_PASSED"
    echo "Failed:      $E2E_TESTS_FAILED"
    echo "Duration:    ${duration}s"
    echo ""

    if [ "$E2E_TESTS_FAILED" -eq 0 ]; then
        log_success "✅ All end-to-end tests passed!"
        return 0
    else
        log_error "❌ Some end-to-end tests failed"
        if [ "$E2E_KEEP_ON_FAILURE" = "true" ]; then
            log_info "Test artifacts preserved in: $E2E_WORKSPACE"
        fi
        return 1
    fi
}

# Initialize test framework
init_test_framework() {
    E2E_START_TIME=$(date +%s)
    E2E_TESTS_RUN=0
    E2E_TESTS_PASSED=0
    E2E_TESTS_FAILED=0

    log_verbose "Test framework initialized"
    log_verbose "Verbosity: $E2E_VERBOSITY"
    log_verbose "Timeout: ${E2E_TIMEOUT}s"
    log_verbose "Workspace: $E2E_WORKSPACE"
    log_verbose "Keep on failure: $E2E_KEEP_ON_FAILURE"
}

# Export functions for use in test scripts
export -f log_info log_success log_error log_warning log_verbose log_stage
export -f assert_file_exists assert_dir_exists assert_command_success
export -f assert_string_contains assert_file_contains assert_not_file_contains
export -f assert_process_running assert_process_not_running
export -f create_workspace cleanup_workspace
export -f wait_for_process wait_for_file kill_process_gracefully
export -f print_test_summary init_test_framework
