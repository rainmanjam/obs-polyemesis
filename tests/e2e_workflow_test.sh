#!/bin/bash
# End-to-End Workflow Tests
# Tests complete real-world workflows across multiple features

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

log_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

test_start() {
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    echo ""
    echo "========================================================================"
    echo -e "${YELLOW}WORKFLOW TEST #${TESTS_TOTAL}: $1${NC}"
    echo "========================================================================"
}

test_pass() {
    TESTS_PASSED=$((TESTS_PASSED + 1))
    log_info "✓ WORKFLOW PASS: $1"
}

test_fail() {
    TESTS_FAILED=$((TESTS_FAILED + 1))
    log_error "✗ WORKFLOW FAIL: $1"
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
    log_step "Authenticating with Restreamer..."

    RESPONSE=$(curl -s -X POST "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/login" \
        -H "Content-Type: application/json" \
        -d "{\"username\":\"${RESTREAMER_USER}\",\"password\":\"${RESTREAMER_PASS}\"}")

    ACCESS_TOKEN=$(echo "$RESPONSE" | jq -r '.access_token // empty')

    if [ -n "$ACCESS_TOKEN" ]; then
        log_info "✓ Authentication successful"
        return 0
    else
        log_error "✗ Authentication failed: $RESPONSE"
        return 1
    fi
}

# ========================================================================
# WORKFLOW 1: Content Creator Workflow
# ========================================================================

workflow_content_creator() {
    test_start "Content Creator: Create Stream → Add Metadata → Upload Assets → Monitor"

    local WORKFLOW_SUCCESS=true
    local PROCESS_ID="content_creator_stream_$$"

    # Step 1: Create a streaming process
    log_step "1/5: Creating streaming process..."
    PROCESS_RESP=$(curl -s -X POST "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{
            "id":"'"$PROCESS_ID"'",
            "reference":"content_creator_workflow"
        }')

    if ! echo "$PROCESS_RESP" | jq -e '.id' > /dev/null 2>&1; then
        log_error "Failed to create process"
        WORKFLOW_SUCCESS=false
    else
        log_info "✓ Process created: $PROCESS_ID"
    fi

    # Step 2: Add descriptive metadata
    log_step "2/5: Adding stream metadata (title, description, tags)..."

    TITLE=$(echo "My Live Gaming Stream - $(date +%Y-%m-%d)" | jq -R .)
    DESC=$(echo "Playing Minecraft with viewers! Drop by and say hi!" | jq -R .)
    TAGS=$(echo "gaming,minecraft,live,interactive" | jq -R .)

    curl -s -X PUT "${BASE_URL}/process/${PROCESS_ID}/metadata/title" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$TITLE" > /dev/null

    curl -s -X PUT "${BASE_URL}/process/${PROCESS_ID}/metadata/description" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$DESC" > /dev/null

    curl -s -X PUT "${BASE_URL}/process/${PROCESS_ID}/metadata/tags" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$TAGS" > /dev/null

    # Verify metadata was stored
    VERIFY_TITLE=$(curl -s -X GET "${BASE_URL}/process/${PROCESS_ID}/metadata/title" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    if echo "$VERIFY_TITLE" | jq -e '. | contains("Gaming Stream")' > /dev/null 2>&1; then
        log_info "✓ Metadata added successfully"
    else
        log_error "Failed to verify metadata"
        WORKFLOW_SUCCESS=false
    fi

    # Step 3: Upload stream assets (thumbnail, overlay)
    log_step "3/5: Uploading stream assets..."

    # Create mock thumbnail
    echo "Mock Thumbnail Data $(date)" > /tmp/stream_thumbnail.png
    curl -s -X PUT "${BASE_URL}/fs/disk/stream_assets/thumbnail_${PROCESS_ID}.png" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: image/png" \
        --data-binary @/tmp/stream_thumbnail.png > /dev/null

    # Create mock overlay
    echo "Mock Overlay Data $(date)" > /tmp/stream_overlay.png
    curl -s -X PUT "${BASE_URL}/fs/disk/stream_assets/overlay_${PROCESS_ID}.png" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: image/png" \
        --data-binary @/tmp/stream_overlay.png > /dev/null

    log_info "✓ Stream assets uploaded"

    # Step 4: Verify uploaded files
    log_step "4/5: Verifying uploaded assets..."

    FILES=$(curl -s -X GET "${BASE_URL}/fs/disk" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    if echo "$FILES" | jq -e '.[] | select(.name | contains("thumbnail_"))' > /dev/null 2>&1; then
        log_info "✓ Thumbnail verified"
    else
        log_warn "Could not verify thumbnail"
    fi

    # Step 5: Monitor process state
    log_step "5/5: Checking stream status..."

    STATE=$(curl -s -X GET "${BASE_URL}/process/${PROCESS_ID}/state" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    if echo "$STATE" | jq -e '.order' > /dev/null 2>&1; then
        ORDER=$(echo "$STATE" | jq -r '.order')
        log_info "✓ Stream state: $ORDER"
    else
        log_error "Failed to get stream state"
        WORKFLOW_SUCCESS=false
    fi

    # Cleanup
    log_step "Cleaning up workflow resources..."
    curl -s -X DELETE "${BASE_URL}/process/${PROCESS_ID}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null
    curl -s -X DELETE "${BASE_URL}/fs/disk/stream_assets/thumbnail_${PROCESS_ID}.png" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null
    curl -s -X DELETE "${BASE_URL}/fs/disk/stream_assets/overlay_${PROCESS_ID}.png" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null
    rm -f /tmp/stream_thumbnail.png /tmp/stream_overlay.png

    if [ "$WORKFLOW_SUCCESS" = true ]; then
        test_pass "Content Creator Workflow"
    else
        test_fail "Content Creator Workflow"
    fi
}

# ========================================================================
# WORKFLOW 2: Multi-Platform Streaming
# ========================================================================

workflow_multi_platform() {
    test_start "Multi-Platform: Stream to YouTube, Twitch, Facebook Simultaneously"

    local WORKFLOW_SUCCESS=true
    local PROCESS_ID="multistream_$$"

    # Step 1: Create base process
    log_step "1/4: Creating multi-platform streaming process..."
    PROCESS_RESP=$(curl -s -X POST "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{
            "id":"'"$PROCESS_ID"'",
            "reference":"multistream_workflow"
        }')

    if ! echo "$PROCESS_RESP" | jq -e '.id' > /dev/null 2>&1; then
        log_error "Failed to create process"
        WORKFLOW_SUCCESS=false
    else
        log_info "✓ Base process created"
    fi

    # Step 2: Add platform-specific metadata
    log_step "2/4: Configuring platform-specific settings..."

    PLATFORMS=$(cat <<EOF
{
  "youtube": {"stream_key": "yt_key_12345", "resolution": "1080p"},
  "twitch": {"stream_key": "twitch_key_67890", "resolution": "1080p"},
  "facebook": {"stream_key": "fb_key_abcde", "resolution": "720p"}
}
EOF
)

    PLATFORMS_JSON=$(echo "$PLATFORMS" | jq -c . | jq -R .)

    curl -s -X PUT "${BASE_URL}/process/${PROCESS_ID}/metadata/platforms" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$PLATFORMS_JSON" > /dev/null

    log_info "✓ Platform metadata configured"

    # Step 3: Record stream configuration
    log_step "3/4: Storing stream configuration..."

    CONFIG=$(echo "Multi-platform stream configured on $(date)" | jq -R .)
    curl -s -X PUT "${BASE_URL}/process/${PROCESS_ID}/metadata/notes" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$CONFIG" > /dev/null

    log_info "✓ Configuration recorded"

    # Step 4: Verify all metadata
    log_step "4/4: Verifying multi-platform configuration..."

    PLATFORMS_VERIFY=$(curl -s -X GET \
        "${BASE_URL}/process/${PROCESS_ID}/metadata/platforms" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    if echo "$PLATFORMS_VERIFY" | jq -e '. | contains("youtube")' > /dev/null 2>&1; then
        log_info "✓ Multi-platform configuration verified"
    else
        log_error "Failed to verify configuration"
        WORKFLOW_SUCCESS=false
    fi

    # Cleanup
    log_step "Cleaning up workflow resources..."
    curl -s -X DELETE "${BASE_URL}/process/${PROCESS_ID}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null

    if [ "$WORKFLOW_SUCCESS" = true ]; then
        test_pass "Multi-Platform Streaming Workflow"
    else
        test_fail "Multi-Platform Streaming Workflow"
    fi
}

# ========================================================================
# WORKFLOW 3: Recording Management
# ========================================================================

workflow_recording_management() {
    test_start "Recording Management: Upload → Tag → Download → Archive"

    local WORKFLOW_SUCCESS=true

    # Step 1: Upload recording
    log_step "1/5: Uploading stream recording..."

    RECORDING_NAME="recording_$(date +%Y%m%d_%H%M%S).mp4"
    echo "Mock Recording Data - $(date)" > /tmp/${RECORDING_NAME}

    UPLOAD_RESP=$(curl -s -w "\nHTTP_CODE:%{http_code}" -X PUT \
        "${BASE_URL}/fs/disk/recordings/${RECORDING_NAME}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: video/mp4" \
        --data-binary @/tmp/${RECORDING_NAME})

    HTTP_CODE=$(echo "$UPLOAD_RESP" | grep "HTTP_CODE:" | cut -d: -f2)

    if [ "$HTTP_CODE" = "200" ]; then
        log_info "✓ Recording uploaded: ${RECORDING_NAME}"
    else
        log_error "Failed to upload recording (HTTP $HTTP_CODE)"
        WORKFLOW_SUCCESS=false
    fi

    # Step 2: Create catalog entry with metadata
    log_step "2/5: Creating catalog entry..."

    CATALOG_ID="catalog_${RECORDING_NAME%.mp4}"

    curl -s -X POST "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{
            "id":"'"$CATALOG_ID"'",
            "reference":"recording_catalog"
        }' > /dev/null

    # Tag the recording
    TAGS=$(echo "archived,gaming,highlight" | jq -R .)
    TITLE=$(echo "Epic Stream Moments - $(date +%Y-%m-%d)" | jq -R .)

    curl -s -X PUT "${BASE_URL}/process/${CATALOG_ID}/metadata/tags" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$TAGS" > /dev/null

    curl -s -X PUT "${BASE_URL}/process/${CATALOG_ID}/metadata/title" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$TITLE" > /dev/null

    FILE_PATH=$(echo "/recordings/${RECORDING_NAME}" | jq -R .)
    curl -s -X PUT "${BASE_URL}/process/${CATALOG_ID}/metadata/file_path" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$FILE_PATH" > /dev/null

    log_info "✓ Catalog entry created with tags"

    # Step 3: List and verify recordings
    log_step "3/5: Verifying recording in catalog..."

    FILES=$(curl -s -X GET "${BASE_URL}/fs/disk" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    if echo "$FILES" | jq -e ".[] | select(.name == \"${RECORDING_NAME}\")" > /dev/null 2>&1; then
        log_info "✓ Recording found in filesystem"
    else
        log_warn "Recording not found in listing"
    fi

    # Step 4: Download for backup
    log_step "4/5: Downloading recording for backup..."

    curl -s -X GET "${BASE_URL}/fs/disk/recordings/${RECORDING_NAME}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -o /tmp/downloaded_${RECORDING_NAME}

    if [ -f "/tmp/downloaded_${RECORDING_NAME}" ]; then
        log_info "✓ Recording downloaded successfully"
    else
        log_error "Failed to download recording"
        WORKFLOW_SUCCESS=false
    fi

    # Step 5: Archive (mark as archived in metadata)
    log_step "5/5: Marking as archived..."

    ARCHIVE_DATE=$(echo "Archived on $(date)" | jq -R .)
    curl -s -X PUT "${BASE_URL}/process/${CATALOG_ID}/metadata/archived" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$ARCHIVE_DATE" > /dev/null

    log_info "✓ Recording marked as archived"

    # Cleanup
    log_step "Cleaning up workflow resources..."
    curl -s -X DELETE "${BASE_URL}/process/${CATALOG_ID}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null
    curl -s -X DELETE "${BASE_URL}/fs/disk/recordings/${RECORDING_NAME}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null
    rm -f /tmp/${RECORDING_NAME} /tmp/downloaded_${RECORDING_NAME}

    if [ "$WORKFLOW_SUCCESS" = true ]; then
        test_pass "Recording Management Workflow"
    else
        test_fail "Recording Management Workflow"
    fi
}

# ========================================================================
# WORKFLOW 4: Production Team Collaboration
# ========================================================================

workflow_team_collaboration() {
    test_start "Team Collaboration: Shared Notes → Asset Management → Version Control"

    local WORKFLOW_SUCCESS=true
    local PROJECT_ID="team_project_$$"

    # Step 1: Create project
    log_step "1/4: Creating team project..."

    curl -s -X POST "${BASE_URL}/process" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d '{
            "id":"'"$PROJECT_ID"'",
            "reference":"team_collaboration"
        }' > /dev/null

    log_info "✓ Team project created"

    # Step 2: Team members add notes
    log_step "2/4: Team members contributing notes..."

    DIRECTOR_NOTE=$(echo "Director: Need more B-roll footage of city skyline" | jq -R .)
    EDITOR_NOTE=$(echo "Editor: Color grading complete, ready for review" | jq -R .)
    PRODUCER_NOTE=$(echo "Producer: Budget approved for additional filming day" | jq -R .)

    curl -s -X PUT "${BASE_URL}/process/${PROJECT_ID}/metadata/director_notes" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$DIRECTOR_NOTE" > /dev/null

    curl -s -X PUT "${BASE_URL}/process/${PROJECT_ID}/metadata/editor_notes" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$EDITOR_NOTE" > /dev/null

    curl -s -X PUT "${BASE_URL}/process/${PROJECT_ID}/metadata/producer_notes" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$PRODUCER_NOTE" > /dev/null

    log_info "✓ Team notes added"

    # Step 3: Upload shared assets
    log_step "3/4: Uploading shared project assets..."

    for asset in script storyboard timeline; do
        echo "Project ${asset} v1.0 - $(date)" > /tmp/project_${asset}.txt
        curl -s -X PUT "${BASE_URL}/fs/disk/team_assets/${PROJECT_ID}_${asset}.txt" \
            -H "Authorization: Bearer ${ACCESS_TOKEN}" \
            -H "Content-Type: text/plain" \
            --data-binary @/tmp/project_${asset}.txt > /dev/null
        rm -f /tmp/project_${asset}.txt
    done

    log_info "✓ Project assets uploaded"

    # Step 4: Track project status
    log_step "4/4: Updating project status..."

    STATUS=$(cat <<EOF
{
  "phase": "post-production",
  "progress": 75,
  "deadline": "2025-12-31",
  "team": ["director", "editor", "producer"]
}
EOF
)

    STATUS_JSON=$(echo "$STATUS" | jq -c . | jq -R .)

    curl -s -X PUT "${BASE_URL}/process/${PROJECT_ID}/metadata/status" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$STATUS_JSON" > /dev/null

    # Verify status
    STATUS_VERIFY=$(curl -s -X GET "${BASE_URL}/process/${PROJECT_ID}/metadata/status" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}")

    if echo "$STATUS_VERIFY" | jq -e '. | contains("post-production")' > /dev/null 2>&1; then
        log_info "✓ Project status tracked"
    else
        log_error "Failed to verify project status"
        WORKFLOW_SUCCESS=false
    fi

    # Cleanup
    log_step "Cleaning up workflow resources..."
    curl -s -X DELETE "${BASE_URL}/process/${PROJECT_ID}" \
        -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null

    for asset in script storyboard timeline; do
        curl -s -X DELETE "${BASE_URL}/fs/disk/team_assets/${PROJECT_ID}_${asset}.txt" \
            -H "Authorization: Bearer ${ACCESS_TOKEN}" > /dev/null
    done

    if [ "$WORKFLOW_SUCCESS" = true ]; then
        test_pass "Team Collaboration Workflow"
    else
        test_fail "Team Collaboration Workflow"
    fi
}

# ========================================================================
# MAIN EXECUTION
# ========================================================================

main() {
    echo "========================================================================"
    echo "            OBS POLYEMESIS - E2E WORKFLOW TESTS"
    echo "========================================================================"
    echo ""

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

    # Run Workflow Tests
    workflow_content_creator
    workflow_multi_platform
    workflow_recording_management
    workflow_team_collaboration

    # Print summary
    echo ""
    echo "========================================================================"
    echo "                        WORKFLOW TEST SUMMARY"
    echo "========================================================================"
    echo "Total Workflows:  ${TESTS_TOTAL}"
    echo -e "Passed:           ${GREEN}${TESTS_PASSED}${NC}"
    echo -e "Failed:           ${RED}${TESTS_FAILED}${NC}"
    echo "========================================================================"

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}All workflows completed successfully!${NC}"
        return 0
    else
        echo -e "${RED}Some workflows failed!${NC}"
        return 1
    fi
}

main "$@"
