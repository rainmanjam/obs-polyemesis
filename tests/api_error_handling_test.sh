#!/bin/bash
# API Error Handling and Edge Case Tests
# Tests error responses, invalid inputs, and edge cases

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

    RESPONSE=$(curl -s -X POST "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/login" \
        -H "Content-Type: application/json" \
        -d "{\"username\":\"${RESTREAMER_USER}\",\"password\":\"${RESTREAMER_PASS}\"}")

    ACCESS_TOKEN=$(echo "$RESPONSE" | jq -r '.access_token // empty')

    if [ -n "$ACCESS_TOKEN" ]; then
        test_pass "Authentication successful"
        return 0
    else
        test_fail "Authentication failed: $RESPONSE"
        return 1
    fi
}

# ========================================================================
# ERROR HANDLING TESTS
# ========================================================================

test_auth_invalid_credentials() {
    test_start "Error Handling - Invalid Credentials"

    RESPONSE=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X POST \
        "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/login" \
        -H "Content-Type: application/json" \
        -d '{"username":"invalid","password":"wrong"}')

    HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE:" | cut -d: -f2)

    if [ "$HTTP_CODE" = "401" ]; then
        test_pass "Correctly rejected invalid credentials (HTTP 401)"
    else
        test_fail "Expected HTTP 401, got HTTP $HTTP_CODE"
    fi
}

test_missing_auth_token() {
    test_start "Error Handling - Missing Authentication Token"

    RESPONSE=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X GET "${BASE_URL}/process")

    HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE:" | cut -d: -f2)

    if [ "$HTTP_CODE" = "401" ]; then
        test_pass "Correctly rejected request without auth token (HTTP 401)"
    else
        test_fail "Expected HTTP 401, got HTTP $HTTP_CODE"
    fi
}

test_invalid_auth_token() {
    test_start "Error Handling - Invalid Authentication Token"

    RESPONSE=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X GET "${BASE_URL}/process" \
        -H "Authorization: Bearer invalid_token_12345")

    HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE:" | cut -d: -f2)

    if [ "$HTTP_CODE" = "401" ]; then
        test_pass "Correctly rejected invalid auth token (HTTP 401)"
    else
        test_fail "Expected HTTP 401, got HTTP $HTTP_CODE"
    fi
}

test_nonexistent_process() {
    test_start "Error Handling - Nonexistent Process ID"

    RESPONSE=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X GET \
        "${BASE_URL}/process/nonexistent_process_999" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE:" | cut -d: -f2)

    if [ "$HTTP_CODE" = "404" ]; then
        test_pass "Correctly returned 404 for nonexistent process"
    else
        test_fail "Expected HTTP 404, got HTTP $HTTP_CODE"
    fi
}

test_nonexistent_filesystem() {
    test_start "Error Handling - Nonexistent Filesystem"

    RESPONSE=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X GET \
        "${BASE_URL}/fs/nonexistent_fs" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE:" | cut -d: -f2)

    if [ "$HTTP_CODE" = "404" ]; then
        test_pass "Correctly returned 404 for nonexistent filesystem"
    else
        test_fail "Expected HTTP 404, got HTTP $HTTP_CODE"
    fi
}

test_invalid_json_payload() {
    test_start "Error Handling - Invalid JSON Payload"

    RESPONSE=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X POST "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{invalid json syntax}')

    HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE:" | cut -d: -f2)

    if [ "$HTTP_CODE" = "400" ]; then
        test_pass "Correctly rejected invalid JSON (HTTP 400)"
    else
        test_fail "Expected HTTP 400, got HTTP $HTTP_CODE"
    fi
}

# ========================================================================
# EDGE CASE TESTS
# ========================================================================

test_empty_metadata_value() {
    test_start "Edge Case - Empty Metadata Value"

    # First create a test process
    PROCESS_RESPONSE=$(curl -s -X POST "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{"id":"test_empty_meta_'"$$"'","reference":"test"}')

    PROCESS_ID=$(echo "$PROCESS_RESPONSE" | jq -r '.id // empty')

    if [ -z "$PROCESS_ID" ]; then
        test_fail "Could not create test process"
        return 1
    fi

    # Try to set empty metadata
    EMPTY_JSON=$(echo "" | jq -R .)
    RESPONSE=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X PUT \
        "${BASE_URL}/process/${PROCESS_ID}/metadata/empty_test" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$EMPTY_JSON")

    HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE:" | cut -d: -f2)

    # Cleanup
    curl -s -X DELETE "${BASE_URL}/process/${PROCESS_ID}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null

    if [ "$HTTP_CODE" = "200" ]; then
        test_pass "Successfully handled empty metadata value"
    else
        test_fail "Failed to handle empty metadata (HTTP $HTTP_CODE)"
    fi
}

test_special_characters_in_metadata() {
    test_start "Edge Case - Special Characters in Metadata"

    # Create test process
    PROCESS_RESPONSE=$(curl -s -X POST "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{"id":"test_special_'"$$"'","reference":"test"}')

    PROCESS_ID=$(echo "$PROCESS_RESPONSE" | jq -r '.id // empty')

    if [ -z "$PROCESS_ID" ]; then
        test_fail "Could not create test process"
        return 1
    fi

    # Try special characters: quotes, newlines, unicode
    SPECIAL_VALUE="Test with \"quotes\", 'apostrophes', newlines\nand unicode: 你好世界 🎬"
    SPECIAL_JSON=$(echo "$SPECIAL_VALUE" | jq -R .)

    RESPONSE=$(curl -s -X PUT \
        "${BASE_URL}/process/${PROCESS_ID}/metadata/special_chars" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$SPECIAL_JSON")

    # Verify we can retrieve it
    GET_RESPONSE=$(curl -s -X GET \
        "${BASE_URL}/process/${PROCESS_ID}/metadata/special_chars" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    # Cleanup
    curl -s -X DELETE "${BASE_URL}/process/${PROCESS_ID}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null

    if echo "$GET_RESPONSE" | jq -e '. | contains("quotes")' > /dev/null 2>&1; then
        test_pass "Successfully handled special characters in metadata"
    else
        test_fail "Failed to handle special characters"
    fi
}

test_large_metadata_value() {
    test_start "Edge Case - Large Metadata Value (10KB)"

    # Create test process
    PROCESS_RESPONSE=$(curl -s -X POST "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{"id":"test_large_meta_'"$$"'","reference":"test"}')

    PROCESS_ID=$(echo "$PROCESS_RESPONSE" | jq -r '.id // empty')

    if [ -z "$PROCESS_ID" ]; then
        test_fail "Could not create test process"
        return 1
    fi

    # Generate 10KB of data
    LARGE_VALUE=$(head -c 10000 /dev/urandom | base64 | tr -d '\n')
    LARGE_JSON=$(echo "$LARGE_VALUE" | jq -R .)

    RESPONSE=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X PUT \
        "${BASE_URL}/process/${PROCESS_ID}/metadata/large_data" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$LARGE_JSON")

    HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE:" | cut -d: -f2)

    # Cleanup
    curl -s -X DELETE "${BASE_URL}/process/${PROCESS_ID}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null

    if [ "$HTTP_CODE" = "200" ]; then
        test_pass "Successfully handled large metadata value (10KB)"
    else
        test_fail "Failed to handle large metadata (HTTP $HTTP_CODE)"
    fi
}

test_concurrent_metadata_updates() {
    test_start "Edge Case - Concurrent Metadata Updates"

    # Create test process
    PROCESS_RESPONSE=$(curl -s -X POST "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{"id":"test_concurrent_'"$$"'","reference":"test"}')

    PROCESS_ID=$(echo "$PROCESS_RESPONSE" | jq -r '.id // empty')

    if [ -z "$PROCESS_ID" ]; then
        test_fail "Could not create test process"
        return 1
    fi

    # Launch 5 concurrent metadata updates
    for i in {1..5}; do
        {
            VALUE=$(echo "Concurrent update $i" | jq -R .)
            curl -s -X PUT \
                "${BASE_URL}/process/${PROCESS_ID}/metadata/concurrent_$i" \
                -H "Authorization: Bearer ${ACCESS_TOKEN}" \
                -H "Content-Type: application/json" \
                -d "$VALUE" > /dev/null
        } &
    done

    # Wait for all background jobs to complete
    wait

    # Verify all updates succeeded
    SUCCESS_COUNT=0
    for i in {1..5}; do
        RESPONSE=$(curl -s -X GET \
            "${BASE_URL}/process/${PROCESS_ID}/metadata/concurrent_$i" \
            -H "Authorization: Bearer ${ACCESS_TOKEN}")

        if echo "$RESPONSE" | jq -e '. | contains("Concurrent update")' > /dev/null 2>&1; then
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        fi
    done

    # Cleanup
    curl -s -X DELETE "${BASE_URL}/process/${PROCESS_ID}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null

    if [ "$SUCCESS_COUNT" -eq 5 ]; then
        test_pass "All 5 concurrent metadata updates succeeded"
    else
        test_fail "Only $SUCCESS_COUNT/5 concurrent updates succeeded"
    fi
}

test_large_file_upload() {
    test_start "Edge Case - Large File Upload (10MB)"

    # Generate 10MB test file
    dd if=/dev/urandom of=/tmp/large_test_file.bin bs=1M count=10 2>/dev/null

    RESPONSE=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X PUT \
        "${BASE_URL}/fs/disk/large_test_file.bin" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/octet-stream" \
        --data-binary @/tmp/large_test_file.bin)

    HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE:" | cut -d: -f2)

    # Cleanup
    curl -s -X DELETE "${BASE_URL}/fs/disk/large_test_file.bin" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null
    rm -f /tmp/large_test_file.bin

    if [ "$HTTP_CODE" = "200" ]; then
        test_pass "Successfully uploaded 10MB file"
    else
        test_fail "Failed to upload large file (HTTP $HTTP_CODE)"
    fi
}

test_file_download_nonexistent() {
    test_start "Edge Case - Download Nonexistent File"

    RESPONSE=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X GET \
        "${BASE_URL}/fs/disk/nonexistent_file_999.txt" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    HTTP_CODE=$(echo "$RESPONSE" | grep "HTTP_CODE:" | cut -d: -f2)

    if [ "$HTTP_CODE" = "404" ]; then
        test_pass "Correctly returned 404 for nonexistent file"
    else
        test_fail "Expected HTTP 404, got HTTP $HTTP_CODE"
    fi
}

# ========================================================================
# STRESS TESTS
# ========================================================================

test_rapid_api_requests() {
    test_start "Stress Test - Rapid API Requests (20 requests/sec)"

    SUCCESS_COUNT=0
    TOTAL_REQUESTS=20

    for i in $(seq 1 $TOTAL_REQUESTS); do
        {
            RESPONSE=$(curl -s -w "%{http_code}" -X GET "${BASE_URL}/process" \
                -H "Authorization: Bearer ${ACCESS_TOKEN}" -o /dev/null)

            if [ "$RESPONSE" = "200" ]; then
                SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
            fi
        } &
    done

    wait

    if [ "$SUCCESS_COUNT" -eq "$TOTAL_REQUESTS" ]; then
        test_pass "All $TOTAL_REQUESTS rapid requests succeeded"
    else
        test_fail "Only $SUCCESS_COUNT/$TOTAL_REQUESTS rapid requests succeeded"
    fi
}

# ========================================================================
# MAIN EXECUTION
# ========================================================================

main() {
    log_info "Starting Error Handling and Edge Case Tests"
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

    # Error Handling Tests
    test_auth_invalid_credentials
    test_missing_auth_token
    test_invalid_auth_token
    test_nonexistent_process
    test_nonexistent_filesystem
    test_invalid_json_payload

    # Edge Case Tests
    test_empty_metadata_value
    test_special_characters_in_metadata
    test_large_metadata_value
    test_concurrent_metadata_updates
    test_large_file_upload
    test_file_download_nonexistent

    # Stress Tests
    test_rapid_api_requests

    # Print summary
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

main "$@"
