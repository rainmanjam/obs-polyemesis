#!/bin/bash
# OBS Polyemesis - Quick Test Runner
# Runs automated tests with proper environment setup

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "==========================================="
echo " OBS Polyemesis - Automated Test Suite"
echo "==========================================="
echo ""

# Load environment variables from .env file if it exists
if [ -f .env ]; then
    echo -e "${GREEN}✓${NC} Loading credentials from .env file"
    set -a  # Export all variables
    source .env
    set +a  # Stop exporting
else
    echo -e "${YELLOW}ℹ${NC}  No .env file found (optional)"
    echo "  Copy .env.example to .env and add your credentials for live tests"
fi
echo ""

# Check Python version
if ! command -v python3 >/dev/null 2>&1; then
    echo -e "${RED}❌ Python 3 not found. Please install Python 3.8+${NC}"
    exit 1
fi

PYTHON_VERSION=$(python3 -c 'import sys; print(".".join(map(str, sys.version_info[:2])))')
echo -e "${GREEN}✓${NC} Python version: $PYTHON_VERSION"

# Check dependencies
if ! python3 -c "import requests" 2>/dev/null; then
    echo -e "${YELLOW}⚠️  Installing dependencies...${NC}"
    pip3 install -r requirements.txt
fi

# Check if Restreamer is running
if curl -s http://localhost:8080/api/v3/about >/dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Restreamer server is running"
else
    echo -e "${RED}❌ Restreamer server not accessible at http://localhost:8080${NC}"
    echo ""
    echo "Start Restreamer with:"
    echo "  docker run -d --name restreamer-test \\"
    echo "    -p 8080:8080 -p 1935:1935 \\"
    echo "    -e RESTREAMER_USERNAME=admin \\"
    echo "    -e RESTREAMER_PASSWORD=admin \\"
    echo "    datarhei/restreamer:latest"
    exit 1
fi

echo ""
echo "==========================================="
echo " Running Basic API Tests"
echo "==========================================="
echo ""

python3 test_polyemesis.py

BASIC_EXIT=$?

# Check if streaming credentials are available
if [ -n "$TWITCH_STREAM_KEY" ] || [ -n "$YOUTUBE_STREAM_KEY" ]; then
    echo ""
    echo "==========================================="
    echo " Running Live Streaming Tests"
    echo "==========================================="
    echo ""

    python3 test_live_streaming.py
    LIVE_EXIT=$?
else
    echo ""
    echo -e "${YELLOW}⚠️  Skipping live streaming tests (no credentials)${NC}"
    echo "To run live tests, set environment variables:"
    echo "  export TWITCH_INGEST_URL='rtmp://live.twitch.tv/app'"
    echo "  export TWITCH_STREAM_KEY='your_key'"
    echo "  export YOUTUBE_INGEST_URL='rtmp://a.rtmp.youtube.com/live2'"
    echo "  export YOUTUBE_STREAM_KEY='your_key'"
    LIVE_EXIT=0
fi

echo ""
echo "==========================================="
echo " Test Run Complete"
echo "==========================================="

if [ $BASIC_EXIT -eq 0 ] && [ $LIVE_EXIT -eq 0 ]; then
    echo -e "${GREEN}✅ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ Some tests failed${NC}"
    exit 1
fi
