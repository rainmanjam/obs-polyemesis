#!/bin/bash
# API Integration Tests for OBS Polyemesis
# Tests all 4 newly implemented features against live Restreamer instance

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
RESTREAMER_HOST="${RESTREAMER_HOST:-localhost}"
RESTREAMER_PORT="${RESTREAMER_PORT:-8080}"
RESTREAMER_USER="${RESTREAMER_USER:-admin}"
RESTREAMER_PASS="${RESTREAMER_PASS:-datarhei}"
BASE_URL="http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/v3"

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_TOTAL=0

# JWT token storage
ACCESS_TOKEN=""
PROCESS_ID=""

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

test_start() {
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    echo ""
    echo -e "${YELLOW}===${NC} Test #${TESTS_TOTAL}: $1"
}

test_pass() {
    TESTS_PASSED=$((TESTS_PASSED + 1))
    log_info "✓ PASS: $1"
}

test_fail() {
    TESTS_FAILED=$((TESTS_FAILED + 1))
    log_error "✗ FAIL: $1"
}

# Wait for Restreamer to be ready
wait_for_restreamer() {
    log_info "Waiting for Restreamer at ${BASE_URL}..."

    for i in {1..30}; do
        # Check if server responds (200 or 401 both mean server is ready)
        HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/about" 2>/dev/null)
        if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "401" ]; then
            log_info "Restreamer is ready! (HTTP $HTTP_CODE)"
            return 0
        fi
        echo -n "."
        sleep 2
    done

    log_error "Restreamer failed to start within 60 seconds"
    return 1
}

# Authentication
authenticate() {
    test_start "Authentication - Login"

    # Note: Login endpoint is /api/login, not /api/v3/login
    RESPONSE=$(curl -s -X POST "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/login" \
        -H "Content-Type: application/json" \
        -d "{\"username\":\"${RESTREAMER_USER}\",\"password\":\"${RESTREAMER_PASS}\"}")

    ACCESS_TOKEN=$(echo "$RESPONSE" | jq -r '.access_token // empty')

    if [ -n "$ACCESS_TOKEN" ]; then
        test_pass "Authentication successful, got access token"
        return 0
    else
        test_fail "Authentication failed: $RESPONSE"
        return 1
    fi
}

# Test Process Management
test_process_list() {
    test_start "Process Management - List Processes"

    RESPONSE=$(curl -s -X GET "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    if echo "$RESPONSE" | jq -e '. | type == "array"' > /dev/null 2>&1; then
        COUNT=$(echo "$RESPONSE" | jq 'length')
        test_pass "Got process list (${COUNT} processes)"
        return 0
    else
        test_fail "Failed to get process list: $RESPONSE"
        return 1
    fi
}

# Create a test process with proper Restreamer v3 config
create_test_process() {
    test_start "Process Management - Create Test Process"

    # Create process with proper Restreamer v3 API format (capitalized fields)
    RESPONSE=$(curl -s -X POST "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{
            "id": "test_process_'"$$"'",
            "reference": "test_ref",
            "input": [
                {
                    "id": "input_0",
                    "address": "testsrc=duration=3600:size=1280x720:rate=30",
                    "options": ["-f", "lavfi", "-re"]
                }
            ],
            "output": [
                {
                    "id": "output_0",
                    "address": "http://localhost:8888/test.m3u8",
                    "options": [
                        "-codec:v", "libx264",
                        "-preset", "ultrafast",
                        "-b:v", "1000k",
                        "-g", "60",
                        "-sc_threshold", "0",
                        "-codec:a", "aac",
                        "-b:a", "128k",
                        "-f", "hls",
                        "-hls_time", "2",
                        "-hls_list_size", "6",
                        "-hls_flags", "delete_segments+append_list"
                    ],
                    "cleanup": []
                }
            ],
            "reconnect": true,
            "reconnect_delay_seconds": 15,
            "autostart": false,
            "stale_timeout_seconds": 30
        }')

    PROCESS_ID=$(echo "$RESPONSE" | jq -r '.id // empty')

    if [ -n "$PROCESS_ID" ]; then
        test_pass "Created test process: ${PROCESS_ID}"

        # Wait a moment for process to initialize
        sleep 2

        # Verify process has input/output configured
        VERIFY_RESPONSE=$(curl -s -X GET "${BASE_URL}/process/${PROCESS_ID}" \
            -H "Authorization: Bearer ${ACCESS_TOKEN}")

        INPUT_COUNT=$(echo "$VERIFY_RESPONSE" | jq '.input | length')
        OUTPUT_COUNT=$(echo "$VERIFY_RESPONSE" | jq '.output | length')

        if [ "$INPUT_COUNT" -gt 0 ] && [ "$OUTPUT_COUNT" -gt 0 ]; then
            log_info "Process configured with ${INPUT_COUNT} input(s) and ${OUTPUT_COUNT} output(s)"
        else
            log_warn "Process created but input/output config may not be populated (input: ${INPUT_COUNT}, output: ${OUTPUT_COUNT})"
        fi

        return 0
    else
        test_fail "Failed to create test process: $RESPONSE"
        return 1
    fi
}

# Feature #7: Metadata Storage Tests
test_metadata_storage() {
    test_start "Feature #7 - Metadata Storage - Set Process Metadata"

    # Set metadata (value must be JSON-encoded)
    JSON_VALUE=$(echo "Test notes for automated testing" | jq -R .)
    RESPONSE=$(curl -s -X PUT "${BASE_URL}/process/${PROCESS_ID}/metadata/notes" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$JSON_VALUE")

    if [ $? -eq 0 ]; then
        test_pass "Set process metadata 'notes'"
    else
        test_fail "Failed to set metadata: $RESPONSE"
        return 1
    fi

    # Get metadata back
    test_start "Feature #7 - Metadata Storage - Get Process Metadata"

    RESPONSE=$(curl -s -X GET "${BASE_URL}/process/${PROCESS_ID}/metadata/notes" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    # Response will be JSON-encoded string, so check for the text within quotes
    if echo "$RESPONSE" | jq -e '. | contains("Test notes for automated testing")' > /dev/null 2>&1; then
        test_pass "Retrieved process metadata successfully"
    else
        test_fail "Failed to retrieve metadata: $RESPONSE"
        return 1
    fi

    # Set multiple metadata fields
    test_start "Feature #7 - Metadata Storage - Set Multiple Metadata"

    # Encode values as JSON strings
    TAGS_JSON=$(echo "test,automated,ci" | jq -R .)
    DESC_JSON=$(echo "Automated test process" | jq -R .)

    curl -s -X PUT "${BASE_URL}/process/${PROCESS_ID}/metadata/tags" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$TAGS_JSON" > /dev/null

    curl -s -X PUT "${BASE_URL}/process/${PROCESS_ID}/metadata/description" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$DESC_JSON" > /dev/null

    test_pass "Set multiple metadata fields"
}

# Feature #8: File System Browser Tests
test_filesystem_browser() {
    test_start "Feature #8 - File System Browser - List Filesystems"

    RESPONSE=$(curl -s -X GET "${BASE_URL}/fs" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    if echo "$RESPONSE" | jq -e '. | type == "array"' > /dev/null 2>&1; then
        FS_COUNT=$(echo "$RESPONSE" | jq 'length')
        test_pass "Listed filesystems (${FS_COUNT} available)"

        # Get first filesystem name
        FIRST_FS=$(echo "$RESPONSE" | jq -r '.[0].name // "disk"')

        # List files
        test_start "Feature #8 - File System Browser - List Files"

        FILES_RESPONSE=$(curl -s -X GET "${BASE_URL}/fs/${FIRST_FS}" \
            -H "Authorization: Bearer ${ACCESS_TOKEN}")

        if echo "$FILES_RESPONSE" | jq -e '. | type == "array"' > /dev/null 2>&1; then
            FILE_COUNT=$(echo "$FILES_RESPONSE" | jq 'length')
            test_pass "Listed files in ${FIRST_FS} (${FILE_COUNT} files)"
        else
            test_fail "Failed to list files: $FILES_RESPONSE"
        fi
    else
        test_fail "Failed to list filesystems: $RESPONSE"
    fi

    # Upload test
    test_start "Feature #8 - File System Browser - Upload File"

    echo "Test file content" > /tmp/test_upload.txt

    UPLOAD_RESPONSE=$(curl -s -X PUT "${BASE_URL}/fs/disk/test_upload.txt" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/octet-stream" \
        --data-binary @/tmp/test_upload.txt)

    if [ $? -eq 0 ]; then
        test_pass "Uploaded test file"

        # Download test
        test_start "Feature #8 - File System Browser - Download File"

        curl -s -X GET "${BASE_URL}/fs/disk/test_upload.txt" \
            -H "Authorization: Bearer ${ACCESS_TOKEN}" \
            -o /tmp/test_download.txt

        if diff /tmp/test_upload.txt /tmp/test_download.txt > /dev/null 2>&1; then
            test_pass "Downloaded file matches uploaded file"
        else
            test_fail "Downloaded file doesn't match uploaded file"
        fi

        # Delete test
        test_start "Feature #8 - File System Browser - Delete File"

        curl -s -X DELETE "${BASE_URL}/fs/disk/test_upload.txt" \
            -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null

        test_pass "Deleted test file"
    else
        test_fail "Failed to upload file"
    fi

    rm -f /tmp/test_upload.txt /tmp/test_download.txt
}

# Feature #9: Dynamic Output Management Tests
test_dynamic_outputs() {
    test_start "Feature #9 - Dynamic Outputs - List Current Outputs"

    RESPONSE=$(curl -s -X GET "${BASE_URL}/process/${PROCESS_ID}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    OUTPUT_COUNT=$(echo "$RESPONSE" | jq '.output | length')
    log_info "Process has ${OUTPUT_COUNT} outputs"

    # Add new output
    test_start "Feature #9 - Dynamic Outputs - Add Output"

    NEW_OUTPUT_RESPONSE=$(curl -s -X POST "${BASE_URL}/process/${PROCESS_ID}/outputs" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{
            "id": "output_test_1",
            "address": "http://localhost:8888/test2.m3u8",
            "options": [
                "-codec:v", "libx264",
                "-preset", "ultrafast",
                "-b:v", "800k",
                "-f", "hls"
            ]
        }')

    if [ $? -eq 0 ]; then
        test_pass "Added new output dynamically"

        # Verify output was added
        test_start "Feature #9 - Dynamic Outputs - Verify Output Added"

        RESPONSE=$(curl -s -X GET "${BASE_URL}/process/${PROCESS_ID}" \
            -H "Authorization: Bearer ${ACCESS_TOKEN}")

        NEW_COUNT=$(echo "$RESPONSE" | jq '.output | length')

        if [ "$NEW_COUNT" -gt "$OUTPUT_COUNT" ]; then
            test_pass "Output count increased from ${OUTPUT_COUNT} to ${NEW_COUNT}"

            # Remove output
            test_start "Feature #9 - Dynamic Outputs - Remove Output"

            curl -s -X DELETE "${BASE_URL}/process/${PROCESS_ID}/outputs/output_test_1" \
                -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null

            test_pass "Removed test output"
        else
            test_fail "Output count did not increase"
        fi
    else
        test_fail "Failed to add output: $NEW_OUTPUT_RESPONSE"
    fi
}

# Feature #10: Playout Management Tests
test_playout_management() {
    test_start "Feature #10 - Playout Management - Get Playout Status"

    RESPONSE=$(curl -s -X GET "${BASE_URL}/process/${PROCESS_ID}/playout/input_0/status" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    if echo "$RESPONSE" | jq -e '.state' > /dev/null 2>&1; then
        STATE=$(echo "$RESPONSE" | jq -r '.state')
        test_pass "Got playout status: ${STATE}"

        # Test reopen input
        test_start "Feature #10 - Playout Management - Reopen Input"

        REOPEN_RESPONSE=$(curl -s -X PUT "${BASE_URL}/process/${PROCESS_ID}/playout/input_0/reopen" \
            -H "Authorization: Bearer ${ACCESS_TOKEN}")

        if [ $? -eq 0 ]; then
            test_pass "Reopened input connection"
        else
            test_fail "Failed to reopen input: $REOPEN_RESPONSE"
        fi
    else
        test_fail "Failed to get playout status: $RESPONSE"
    fi
}

# Process state monitoring
test_process_state() {
    test_start "Extended API - Get Process State"

    RESPONSE=$(curl -s -X GET "${BASE_URL}/process/${PROCESS_ID}/state" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    if echo "$RESPONSE" | jq -e '.order' > /dev/null 2>&1; then
        ORDER=$(echo "$RESPONSE" | jq -r '.order')
        test_pass "Got process state: ${ORDER}"
    else
        test_fail "Failed to get process state: $RESPONSE"
    fi
}

# Cleanup
cleanup_test_process() {
    if [ -n "$PROCESS_ID" ]; then
        log_info "Cleaning up test process: ${PROCESS_ID}"
        curl -s -X DELETE "${BASE_URL}/process/${PROCESS_ID}" \
            -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null
    fi
}

# Print test summary
print_summary() {
    echo ""
    echo "===================================="
    echo "Test Summary"
    echo "===================================="
    echo "Total Tests:  ${TESTS_TOTAL}"
    echo -e "Passed:       ${GREEN}${TESTS_PASSED}${NC}"
    echo -e "Failed:       ${RED}${TESTS_FAILED}${NC}"
    echo "===================================="

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
        return 0
    else
        echo -e "${RED}Some tests failed!${NC}"
        return 1
    fi
}

# Main test execution
main() {
    log_info "Starting OBS Polyemesis API Integration Tests"
    log_info "Testing against: ${BASE_URL}"

    # Wait for Restreamer
    if ! wait_for_restreamer; then
        log_error "Cannot proceed without Restreamer"
        exit 1
    fi

    # Authenticate
    if ! authenticate; then
        log_error "Cannot proceed without authentication"
        exit 1
    fi

    # Run tests
    test_process_list
    create_test_process

    if [ -n "$PROCESS_ID" ]; then
        test_metadata_storage
        test_filesystem_browser
        test_dynamic_outputs
        test_playout_management
        test_process_state

        # Cleanup
        cleanup_test_process
    else
        log_error "No test process created, skipping feature tests"
    fi

    # Print summary
    print_summary
    exit $?
}

# Run main function
main "$@"
