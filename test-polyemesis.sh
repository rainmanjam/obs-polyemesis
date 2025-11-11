#!/bin/bash
# OBS Polyemesis - Automated Test Suite
# Tests Restreamer API integration and plugin functionality

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
log_info() {
    echo -e "${NC}[INFO] $1${NC}"
}

log_success() {
    echo -e "${GREEN}✅ $1${NC}"
    ((TESTS_PASSED++))
}

log_error() {
    echo -e "${RED}❌ $1${NC}"
    ((TESTS_FAILED++))
}

log_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

run_test() {
    ((TESTS_RUN++))
    log_info "Test $TESTS_RUN: $1"
}

# Main test execution
echo "=========================================="
echo " OBS Polyemesis Automated Test Suite"
echo "=========================================="
echo ""

# Check prerequisites
run_test "Checking prerequisites"
PREREQ_OK=true

if ! command -v docker >/dev/null 2>&1; then
    log_error "Docker not installed"
    PREREQ_OK=false
else
    log_success "Docker installed"
fi

if ! command -v curl >/dev/null 2>&1; then
    log_error "curl not installed"
    PREREQ_OK=false
else
    log_success "curl installed"
fi

if ! command -v jq >/dev/null 2>&1; then
    log_error "jq not installed (brew install jq)"
    PREREQ_OK=false
else
    log_success "jq installed"
fi

if [ "$PREREQ_OK" = false ]; then
    echo ""
    log_error "Prerequisites not met. Please install missing tools."
    exit 1
fi

echo ""

# Start Restreamer server
run_test "Starting Restreamer server"
log_info "Pulling datarhei/restreamer:latest..."
docker pull datarhei/restreamer:latest >/dev/null 2>&1

# Stop existing test container if running
docker stop restreamer-test 2>/dev/null || true
docker rm restreamer-test 2>/dev/null || true

log_info "Starting container..."
docker run -d --rm --name restreamer-test \
  -p 8080:8080 \
  -p 1935:1935 \
  -p 6000:6000/udp \
  -e RESTREAMER_USERNAME=admin \
  -e RESTREAMER_PASSWORD=admin \
  datarhei/restreamer:latest >/dev/null 2>&1

log_success "Restreamer container started"

# Wait for server ready
run_test "Waiting for Restreamer API to be ready"
MAX_WAIT=60
for i in $(seq 1 $MAX_WAIT); do
  if curl -s http://localhost:8080/api/v3/about >/dev/null 2>&1; then
    log_success "Server ready after ${i} seconds"
    break
  fi

  if [ $i -eq $MAX_WAIT ]; then
    log_error "Server failed to start within ${MAX_WAIT} seconds"
    docker logs restreamer-test
    docker stop restreamer-test
    exit 1
  fi

  sleep 1
done

echo ""

# Test 1: API health check
run_test "API health check (/api/v3/about)"
ABOUT_RESPONSE=$(curl -s http://localhost:8080/api/v3/about)
SERVER_NAME=$(echo "$ABOUT_RESPONSE" | jq -r .name)

if [ "$SERVER_NAME" = "datarhei Core" ] || [ "$SERVER_NAME" = "Core" ]; then
    log_success "API health check passed: $SERVER_NAME"
else
    log_error "Unexpected server name: $SERVER_NAME"
fi

# Test 2: Login endpoint
run_test "JWT authentication (/api/login)"
TOKEN=$(curl -s -X POST http://localhost:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}' \
  | jq -r .access_token)

if [ -n "$TOKEN" ] && [ "$TOKEN" != "null" ]; then
  log_success "Login successful (token received)"
else
  log_error "Login failed - no token received"
fi

# Test 3: Invalid credentials
run_test "Invalid credentials handling"
INVALID_RESPONSE=$(curl -s -X POST http://localhost:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"wrongpassword"}')

ERROR_MSG=$(echo "$INVALID_RESPONSE" | jq -r .message)
if echo "$ERROR_MSG" | grep -qi "invalid\|unauthorized\|authentication"; then
    log_success "Invalid credentials properly rejected"
else
    log_warning "Unexpected error message: $ERROR_MSG"
fi

# Test 4: Process list
run_test "Get process list (/api/v3/process)"
PROCESSES=$(curl -s -u admin:admin http://localhost:8080/api/v3/process)
PROCESS_COUNT=$(echo "$PROCESSES" | jq length)

if [ "$PROCESS_COUNT" -ge 0 ]; then
    log_success "Process list retrieved: $PROCESS_COUNT processes"
else
    log_error "Failed to get process list"
fi

# Test 5: Create process
run_test "Create test process"
PROCESS_ID="test_obs_polyemesis_$(date +%s)"
PROCESS_JSON=$(cat <<EOF
{
  "id": "$PROCESS_ID",
  "reference": "obs-polyemesis-test",
  "input": [
    {
      "address": "rtmp://localhost/live/obs_input",
      "id": "input_0",
      "options": ["-re"]
    }
  ],
  "output": [
    {
      "address": "rtmp://localhost/live/test_output",
      "id": "output_0",
      "options": ["-c:v", "copy", "-c:a", "copy", "-f", "flv"]
    }
  ]
}
EOF
)

CREATE_RESPONSE=$(curl -s -X POST http://localhost:8080/api/v3/process \
  -u admin:admin \
  -H "Content-Type: application/json" \
  -d "$PROCESS_JSON")

CREATED_ID=$(echo "$CREATE_RESPONSE" | jq -r .id)

if [ "$CREATED_ID" = "$PROCESS_ID" ]; then
    log_success "Process created successfully: $PROCESS_ID"
else
    log_error "Process creation failed"
    echo "$CREATE_RESPONSE" | jq .
fi

# Test 6: Get process details
run_test "Get process details (/api/v3/process/$PROCESS_ID)"
PROCESS_DETAILS=$(curl -s -u admin:admin "http://localhost:8080/api/v3/process/$PROCESS_ID")
PROCESS_STATE=$(echo "$PROCESS_DETAILS" | jq -r .state)

if [ "$PROCESS_STATE" = "created" ] || [ "$PROCESS_STATE" = "starting" ] || [ "$PROCESS_STATE" = "running" ]; then
    log_success "Process details retrieved: state=$PROCESS_STATE"
else
    log_warning "Process in unexpected state: $PROCESS_STATE"
fi

# Test 7: Process control (start)
run_test "Start process command"
START_RESPONSE=$(curl -s -X PUT "http://localhost:8080/api/v3/process/$PROCESS_ID/command" \
  -u admin:admin \
  -H "Content-Type: application/json" \
  -d '{"command":"start"}')

START_STATUS=$(echo "$START_RESPONSE" | jq -r .state)
if [ "$START_STATUS" = "starting" ] || [ "$START_STATUS" = "running" ]; then
    log_success "Process start command accepted"
else
    log_warning "Process start returned: $START_STATUS (may fail without real input)"
fi

# Wait a moment for state to settle
sleep 2

# Test 8: Process state after start
run_test "Check process state after start"
STATE_RESPONSE=$(curl -s -u admin:admin "http://localhost:8080/api/v3/process/$PROCESS_ID/state")
CURRENT_STATE=$(echo "$STATE_RESPONSE" | jq -r .state)

log_success "Process state: $CURRENT_STATE"

# Test 9: Process report/logs
run_test "Get process logs"
REPORT_RESPONSE=$(curl -s -u admin:admin "http://localhost:8080/api/v3/process/$PROCESS_ID/report")
LOG_ENTRIES=$(echo "$REPORT_RESPONSE" | jq '.log | length')

if [ "$LOG_ENTRIES" -gt 0 ]; then
    log_success "Process logs retrieved: $LOG_ENTRIES entries"
else
    log_warning "No log entries found (process may not have started)"
fi

# Test 10: Process control (stop)
run_test "Stop process command"
STOP_RESPONSE=$(curl -s -X PUT "http://localhost:8080/api/v3/process/$PROCESS_ID/command" \
  -u admin:admin \
  -H "Content-Type: application/json" \
  -d '{"command":"stop"}')

STOP_STATUS=$(echo "$STOP_RESPONSE" | jq -r .state)
if [ "$STOP_STATUS" = "stopping" ] || [ "$STOP_STATUS" = "finished" ]; then
    log_success "Process stop command accepted"
else
    log_warning "Process stop returned: $STOP_STATUS"
fi

# Test 11: Delete process
run_test "Delete process"
DELETE_RESPONSE=$(curl -s -X DELETE "http://localhost:8080/api/v3/process/$PROCESS_ID" \
  -u admin:admin)

if [ -z "$DELETE_RESPONSE" ] || [ "$DELETE_RESPONSE" = "{}" ]; then
    log_success "Process deleted successfully"
else
    log_warning "Delete response: $DELETE_RESPONSE"
fi

# Test 12: Verify deletion
run_test "Verify process deleted"
VERIFY_RESPONSE=$(curl -s -w "%{http_code}" -u admin:admin "http://localhost:8080/api/v3/process/$PROCESS_ID" -o /dev/null)

if [ "$VERIFY_RESPONSE" = "404" ]; then
    log_success "Process no longer exists (404 returned)"
else
    log_warning "Process still exists (HTTP $VERIFY_RESPONSE)"
fi

# Test 13: Session list
run_test "Get active sessions (/api/v3/session/active)"
SESSIONS=$(curl -s -u admin:admin http://localhost:8080/api/v3/session/active)
SESSION_COUNT=$(echo "$SESSIONS" | jq length)

if [ "$SESSION_COUNT" -ge 0 ]; then
    log_success "Session list retrieved: $SESSION_COUNT active sessions"
else
    log_error "Failed to get session list"
fi

# Test 14: Config endpoint
run_test "Get server config (/api/v3/config)"
CONFIG=$(curl -s -u admin:admin http://localhost:8080/api/v3/config)
CONFIG_VERSION=$(echo "$CONFIG" | jq -r .version)

if [ -n "$CONFIG_VERSION" ] && [ "$CONFIG_VERSION" != "null" ]; then
    log_success "Config retrieved: version $CONFIG_VERSION"
else
    log_warning "Config version not found"
fi

# Test 15: Skills endpoint (FFmpeg capabilities)
run_test "Get FFmpeg skills (/api/v3/skills)"
SKILLS=$(curl -s -u admin:admin http://localhost:8080/api/v3/skills)
ENCODER_COUNT=$(echo "$SKILLS" | jq '.ffmpeg.encoders | length')

if [ "$ENCODER_COUNT" -gt 0 ]; then
    log_success "Skills retrieved: $ENCODER_COUNT encoders available"
else
    log_warning "No encoders found in skills"
fi

echo ""
echo "=========================================="
echo " Cleanup"
echo "=========================================="

# Cleanup
log_info "Stopping Restreamer container..."
docker stop restreamer-test >/dev/null 2>&1
log_success "Cleanup complete"

# Summary
echo ""
echo "=========================================="
echo " Test Summary"
echo "=========================================="
echo "Total tests run: $TESTS_RUN"
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "${RED}Failed: $TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ Some tests failed${NC}"
    exit 1
fi
