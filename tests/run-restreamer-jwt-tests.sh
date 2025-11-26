#!/bin/bash
# OBS Polyemesis - Restreamer Integration Tests with JWT Authentication
# Handles JWT token-based authentication for Restreamer API

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SECRETS_FILE="$PROJECT_ROOT/.secrets"

# Test results
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

log_section() {
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo -e "${CYAN}$1${NC}"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
}

# Check if secrets file exists
if [ ! -f "$SECRETS_FILE" ]; then
    echo -e "${RED}Error: .secrets file not found!${NC}"
    exit 1
fi

# Load secrets
log_info "Loading credentials from .secrets..."
source "$SECRETS_FILE"

# Build base URL
if [ "$RESTREAMER_USE_HTTPS" = "true" ]; then
    BASE_URL="https://${RESTREAMER_HOST}:${RESTREAMER_PORT}"
else
    BASE_URL="http://${RESTREAMER_HOST}:${RESTREAMER_PORT}"
fi

log_success "Credentials loaded"
log_info "Server: $BASE_URL"
echo ""

log_section "Restreamer JWT Authentication Tests"

# Test 1: Login and get JWT token
log_info "[1/7] Authenticating and obtaining JWT token..."
TESTS_RUN=$((TESTS_RUN + 1))

LOGIN_RESPONSE=$(curl -s -X POST "${BASE_URL}/api/login" \
    -H "Content-Type: application/json" \
    -d "{\"username\":\"${RESTREAMER_USERNAME}\",\"password\":\"${RESTREAMER_PASSWORD}\"}")

if echo "$LOGIN_RESPONSE" | jq -e '.access_token' >/dev/null 2>&1; then
    JWT_TOKEN=$(echo "$LOGIN_RESPONSE" | jq -r '.access_token')
    log_success "JWT token obtained successfully"
elif echo "$LOGIN_RESPONSE" | jq -e 'has("code")' >/dev/null 2>&1; then
    ERROR_MSG=$(echo "$LOGIN_RESPONSE" | jq -r '.message')
    log_fail "Authentication failed: $ERROR_MSG"
    log_info "Please verify credentials in .secrets file"
    log_info "Current username: $RESTREAMER_USERNAME"
    exit 1
else
    log_fail "Unexpected response from login endpoint"
    echo "Response: $LOGIN_RESPONSE"
    exit 1
fi

# Test 2: Get server info with JWT
log_info "[2/7] Testing authenticated API access..."
TESTS_RUN=$((TESTS_RUN + 1))

SERVER_INFO=$(curl -s "${BASE_URL}/api/v3" \
    -H "Authorization: Bearer ${JWT_TOKEN}")

if echo "$SERVER_INFO" | jq -e '.version' >/dev/null 2>&1; then
    VERSION=$(echo "$SERVER_INFO" | jq -r '.version')
    NAME=$(echo "$SERVER_INFO" | jq -r '.name')
    log_success "Server info retrieved (Name: $NAME, Version: $VERSION)"
else
    log_fail "Could not retrieve server info with JWT"
fi

# Test 3: List processes
log_info "[3/7] Testing process list retrieval..."
TESTS_RUN=$((TESTS_RUN + 1))

PROCESSES=$(curl -s "${BASE_URL}/api/v3/process" \
    -H "Authorization: Bearer ${JWT_TOKEN}")

if echo "$PROCESSES" | jq -e 'type == "array"' >/dev/null 2>&1; then
    COUNT=$(echo "$PROCESSES" | jq 'length')
    log_success "Process list retrieved ($COUNT processes)"

    if [ "$COUNT" -gt 0 ]; then
        echo "  Active processes:"
        echo "$PROCESSES" | jq -r '.[] | "  → \(.reference // .id): \(.state.order)"' | head -5
    fi
else
    log_fail "Could not retrieve process list"
fi

# Test 4: Get server capabilities/skills
log_info "[4/7] Testing server capabilities..."
TESTS_RUN=$((TESTS_RUN + 1))

SKILLS=$(curl -s "${BASE_URL}/api/v3/skills" \
    -H "Authorization: Bearer ${JWT_TOKEN}")

if echo "$SKILLS" | jq -e 'has("ffmpeg")' >/dev/null 2>&1; then
    FFMPEG_VERSION=$(echo "$SKILLS" | jq -r '.ffmpeg.version')
    log_success "Server capabilities retrieved (FFmpeg: $FFMPEG_VERSION)"
else
    log_fail "Could not retrieve server capabilities"
fi

# Test 5: Check filesystem (memfs for HLS output)
log_info "[5/7] Testing filesystem access..."
TESTS_RUN=$((TESTS_RUN + 1))

FS_INFO=$(curl -s "${BASE_URL}/api/v3/fs" \
    -H "Authorization: Bearer ${JWT_TOKEN}")

if echo "$FS_INFO" | jq -e 'type == "array"' >/dev/null 2>&1; then
    FS_COUNT=$(echo "$FS_INFO" | jq 'length')
    log_success "Filesystem info retrieved ($FS_COUNT filesystems)"

    if [ "$FS_COUNT" -gt 0 ]; then
        echo "  Available filesystems:"
        echo "$FS_INFO" | jq -r '.[] | "  → \(.name): \(.type)"' | head -3
    fi
else
    log_fail "Could not retrieve filesystem info"
fi

# Test 6: Check metadata/config
log_info "[6/7] Testing server metadata..."
TESTS_RUN=$((TESTS_RUN + 1))

METADATA=$(curl -s "${BASE_URL}/api/v3/metadata" \
    -H "Authorization: Bearer ${JWT_TOKEN}")

if echo "$METADATA" | jq -e 'has("name")' >/dev/null 2>&1; then
    METADATA_NAME=$(echo "$METADATA" | jq -r '.name // "N/A"')
    log_success "Server metadata retrieved (Name: $METADATA_NAME)"
else
    log_fail "Could not retrieve server metadata"
fi

# Test 7: Test process creation (dry run - don't actually create)
log_info "[7/7] Verifying process creation endpoint..."
TESTS_RUN=$((TESTS_RUN + 1))

# Just verify we can access the endpoint with proper auth
# Don't actually create a process
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/api/v3/process" \
    -H "Authorization: Bearer ${JWT_TOKEN}")

if [ "$HTTP_CODE" = "200" ]; then
    log_success "Process creation endpoint accessible"
else
    log_fail "Process creation endpoint returned HTTP $HTTP_CODE"
fi

# ==========================================
# Summary
# ==========================================

log_section "Test Summary"

echo "Tests run:    $TESTS_RUN"
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    PASS_RATE=100
else
    PASS_RATE=$((TESTS_PASSED * 100 / TESTS_RUN))
fi

echo "Pass rate: ${PASS_RATE}%"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All integration tests passed!${NC}"
    echo ""
    echo "Server is ready for streaming tests:"
    echo "  → JWT authentication working"
    echo "  → API endpoints accessible"
    echo "  → Ready for vertical/horizontal streaming tests"
    echo "  → Ready for multi-destination tests"
    echo ""
    echo "JWT Token (valid for session):"
    echo "  ${JWT_TOKEN:0:50}..."
    exit 0
else
    echo -e "${RED}❌ Some integration tests failed${NC}"
    echo ""
    echo "Troubleshooting:"
    echo "  1. Verify credentials in .secrets are correct"
    echo "  2. Check server is accessible: curl $BASE_URL"
    echo "  3. Verify JWT authentication is enabled on server"
    echo "  4. Check firewall/network settings"
    exit 1
fi
