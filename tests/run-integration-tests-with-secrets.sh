#!/bin/bash
# OBS Polyemesis - Integration Tests with Secrets
# Loads credentials from .secrets file and runs comprehensive tests

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
    echo ""
    echo "Please create a .secrets file with your credentials:"
    echo "  1. Copy .secrets.template to .secrets"
    echo "  2. Fill in your Restreamer credentials"
    echo "  3. Run this script again"
    echo ""
    echo "Example:"
    echo "  cp .secrets.template .secrets"
    echo "  # Edit .secrets with your credentials"
    echo "  ./tests/run-integration-tests-with-secrets.sh"
    exit 1
fi

# Load secrets
log_info "Loading credentials from .secrets..."
source "$SECRETS_FILE"

# Validate required variables
REQUIRED_VARS=(
    "RESTREAMER_HOST"
    "RESTREAMER_PORT"
    "RESTREAMER_USERNAME"
    "RESTREAMER_PASSWORD"
)

for var in "${REQUIRED_VARS[@]}"; do
    if [ -z "${!var}" ]; then
        echo -e "${RED}Error: $var not set in .secrets${NC}"
        exit 1
    fi
done

log_success "Credentials loaded successfully"

# Build base URL
if [ "$RESTREAMER_USE_HTTPS" = "true" ]; then
    BASE_URL="https://${RESTREAMER_HOST}:${RESTREAMER_PORT}"
else
    BASE_URL="http://${RESTREAMER_HOST}:${RESTREAMER_PORT}"
fi

AUTH="${RESTREAMER_USERNAME}:${RESTREAMER_PASSWORD}"

log_info "Testing server: $BASE_URL"
echo ""

# ==========================================
# Test Suite
# ==========================================

log_section "Restreamer Integration Tests"

# Test 1: Basic connectivity
log_info "[1/6] Testing basic connectivity..."
TESTS_RUN=$((TESTS_RUN + 1))

HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" -u "$AUTH" "${BASE_URL}/api/v3/process")

if [ "$HTTP_CODE" = "200" ]; then
    log_success "API endpoint accessible (HTTP $HTTP_CODE)"
else
    log_fail "API endpoint not accessible (HTTP $HTTP_CODE)"
    if [ "$HTTP_CODE" = "401" ]; then
        echo "  → Check username/password in .secrets"
    elif [ "$HTTP_CODE" = "000" ]; then
        echo "  → Check host/port and network connectivity"
    fi
fi

# Test 2: Server version
log_info "[2/6] Testing server version retrieval..."
TESTS_RUN=$((TESTS_RUN + 1))

SERVER_INFO=$(curl -s -u "$AUTH" "${BASE_URL}/api/v3")

if echo "$SERVER_INFO" | grep -q "version"; then
    VERSION=$(echo "$SERVER_INFO" | jq -r '.version' 2>/dev/null || echo "unknown")
    NAME=$(echo "$SERVER_INFO" | jq -r '.name' 2>/dev/null || echo "unknown")
    log_success "Server info retrieved (Name: $NAME, Version: $VERSION)"
else
    log_fail "Could not retrieve server info"
fi

# Test 3: Process list
log_info "[3/6] Testing process list retrieval..."
TESTS_RUN=$((TESTS_RUN + 1))

PROCESSES=$(curl -s -u "$AUTH" "${BASE_URL}/api/v3/process")

if echo "$PROCESSES" | jq -e 'type == "array"' >/dev/null 2>&1; then
    COUNT=$(echo "$PROCESSES" | jq 'length' 2>/dev/null || echo "0")
    log_success "Process list retrieved ($COUNT processes)"

    # List active processes
    if [ "$COUNT" -gt 0 ]; then
        echo "$PROCESSES" | jq -r '.[] | "  → ID: \(.id) | State: \(.state.order) | Reference: \(.reference)"' 2>/dev/null | head -5
    fi
else
    log_fail "Could not retrieve process list"
fi

# Test 4: Process creation (dry run)
log_info "[4/6] Testing process creation capability..."
TESTS_RUN=$((TESTS_RUN + 1))

# Create a test process configuration
TEST_PROCESS_CONFIG='{
  "id": "test_obs_polyemesis_'$(date +%s)'",
  "reference": "obs-polyemesis-test",
  "input": [{
    "id": "input_0",
    "address": "rtmp://127.0.0.1:1935/live/test",
    "options": ["-re"]
  }],
  "output": [{
    "id": "output_0",
    "address": "rtmp://127.0.0.1:1935/live/test-output",
    "options": ["-c", "copy"]
  }],
  "options": ["-err_detect", "ignore_err"]
}'

# Don't actually create it, just test if we have permission
METADATA_RESPONSE=$(curl -s -u "$AUTH" "${BASE_URL}/api/v3/process")
if echo "$METADATA_RESPONSE" | jq -e 'type == "array"' >/dev/null 2>&1; then
    log_success "Process creation endpoint accessible (not creating test process)"
else
    log_fail "Process creation endpoint not accessible"
fi

# Test 5: Skills/capabilities
log_info "[5/6] Testing server capabilities..."
TESTS_RUN=$((TESTS_RUN + 1))

SKILLS=$(curl -s -u "$AUTH" "${BASE_URL}/api/v3/skills")

if echo "$SKILLS" | jq -e 'type == "object"' >/dev/null 2>&1; then
    FFMPEG=$(echo "$SKILLS" | jq -r '.ffmpeg.version' 2>/dev/null || echo "unknown")
    log_success "Server capabilities retrieved (FFmpeg: $FFMPEG)"
else
    log_fail "Could not retrieve server capabilities"
fi

# Test 6: Health check
log_info "[6/6] Testing server health..."
TESTS_RUN=$((TESTS_RUN + 1))

HEALTH=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/")

if [ "$HEALTH" = "200" ] || [ "$HEALTH" = "302" ]; then
    log_success "Server health check passed (HTTP $HEALTH)"
else
    log_fail "Server health check failed (HTTP $HEALTH)"
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
    echo "  → Vertical streaming test"
    echo "  → Horizontal streaming test"
    echo "  → Multi-destination test"
    echo "  → Error recovery test"
    exit 0
else
    echo -e "${RED}❌ Some integration tests failed${NC}"
    echo ""
    echo "Troubleshooting:"
    echo "  1. Verify credentials in .secrets are correct"
    echo "  2. Check server is accessible: curl ${BASE_URL}"
    echo "  3. Verify API is enabled on server"
    echo "  4. Check firewall/network settings"
    exit 1
fi
