#!/bin/bash
set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

log_info "OBS Polyemesis - Integration Tests"
echo "========================================"

# Wait for services to be ready
log_info "Waiting for Restreamer to be ready..."
for i in {1..30}; do
    if curl -f -s "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/v3/ping" > /dev/null; then
        log_success "Restreamer is ready"
        break
    fi
    if [ $i -eq 30 ]; then
        log_error "Restreamer did not become ready in time"
        exit 1
    fi
    sleep 2
done

log_info "Waiting for RTMP server to be ready..."
for i in {1..15}; do
    if curl -f -s "http://${RTMP_HOST}:8088/stat" > /dev/null; then
        log_success "RTMP server is ready"
        break
    fi
    if [ $i -eq 15 ]; then
        log_error "RTMP server did not become ready in time"
        exit 1
    fi
    sleep 1
done

# Build plugin
log_info "Building OBS plugin..."
cd /workspace
mkdir -p build_integration
cd build_integration
cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_FRONTEND_API=ON \
    -DENABLE_QT=OFF \
    -DENABLE_TESTING=ON

cmake --build . --target obs-polyemesis

log_success "Plugin built successfully"

# Run integration tests
log_info "Running integration test suite..."

cd tests
mkdir -p /results

# Test 1: Connect to Restreamer
log_info "Test 1: API Connection"
if curl -u "${RESTREAMER_USERNAME}:${RESTREAMER_PASSWORD}" \
    -s "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/v3/config" > /results/config.json; then
    log_success "✓ Connected to Restreamer API"
else
    log_error "✗ Failed to connect to Restreamer API"
    exit 1
fi

# Test 2: Create process
log_info "Test 2: Create streaming process"
PROCESS_PAYLOAD='{
  "id": "integration-test-stream",
  "reference": "test",
  "input": [
    {
      "id": "input_0",
      "address": "rtmp://'"${RTMP_HOST}"':1935/live/test",
      "options": ["-re"]
    }
  ],
  "output": [
    {
      "id": "output_0",
      "address": "rtmp://'"${RTMP_HOST}"':1935/output/test",
      "options": []
    }
  ]
}'

if curl -u "${RESTREAMER_USERNAME}:${RESTREAMER_PASSWORD}" \
    -X POST \
    -H "Content-Type: application/json" \
    -d "$PROCESS_PAYLOAD" \
    -s "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/v3/process" \
    > /results/process.json; then
    log_success "✓ Created streaming process"
else
    log_error "✗ Failed to create streaming process"
    exit 1
fi

# Test 3: Generate test stream with FFmpeg
log_info "Test 3: Generate test stream"
timeout 10 ffmpeg -re -f lavfi -i testsrc=size=640x480:rate=30 \
    -f lavfi -i sine=frequency=1000:sample_rate=48000 \
    -c:v libx264 -preset ultrafast -tune zerolatency -b:v 500k \
    -c:a aac -b:a 128k \
    -f flv "rtmp://${RTMP_HOST}:1935/live/test" \
    &> /results/ffmpeg.log &

FFMPEG_PID=$!
sleep 5  # Let stream start

if kill -0 $FFMPEG_PID 2>/dev/null; then
    log_success "✓ Test stream is running"
else
    log_error "✗ Test stream failed to start"
    cat /results/ffmpeg.log
    exit 1
fi

# Test 4: Verify stream processing
log_info "Test 4: Verify stream is being processed"
sleep 3  # Wait for processing to start

if curl -u "${RESTREAMER_USERNAME}:${RESTREAMER_PASSWORD}" \
    -s "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/v3/process/integration-test-stream" \
    > /results/process_status.json; then

    STATE=$(jq -r '.state' /results/process_status.json)
    if [ "$STATE" = "running" ]; then
        log_success "✓ Process is running"
    else
        log_error "✗ Process state is: $STATE (expected: running)"
        cat /results/process_status.json
        exit 1
    fi
else
    log_error "✗ Failed to get process status"
    exit 1
fi

# Test 5: Stop process
log_info "Test 5: Stop streaming process"
if curl -u "${RESTREAMER_USERNAME}:${RESTREAMER_PASSWORD}" \
    -X PUT \
    -H "Content-Type: application/json" \
    -d '{"command":"stop"}' \
    -s "http://${RESTREAMER_HOST}:${RESTREAMER_PORT}/api/v3/process/integration-test-stream/command" \
    > /dev/null; then
    log_success "✓ Stopped streaming process"
else
    log_error "✗ Failed to stop streaming process"
    exit 1
fi

# Cleanup
kill $FFMPEG_PID 2>/dev/null || true

log_success "All integration tests passed!"
echo ""
echo "Results saved to /results/"
echo "  - config.json: Restreamer configuration"
echo "  - process.json: Process creation response"
echo "  - process_status.json: Process status"
echo "  - ffmpeg.log: FFmpeg stream logs"
