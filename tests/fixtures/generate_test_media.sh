#!/bin/bash
# Generate test media files for API integration testing
# Requires FFmpeg to be installed

set -e

MEDIA_DIR="$(cd "$(dirname "$0")/media" && pwd)"

echo "Generating test media files in: $MEDIA_DIR"
echo ""

# Check if ffmpeg is available
if ! command -v ffmpeg &> /dev/null; then
    echo "ERROR: ffmpeg is not installed"
    echo "Install with: brew install ffmpeg (macOS) or apt install ffmpeg (Ubuntu)"
    exit 1
fi

cd "$MEDIA_DIR"

# Generate a simple test pattern video (10 seconds, 1280x720, 30fps)
echo "[1/4] Generating test pattern video (test_pattern.mp4)..."
ffmpeg -y -f lavfi -i testsrc=duration=10:size=1280x720:rate=30 \
    -f lavfi -i sine=frequency=1000:duration=10 \
    -c:v libx264 -preset ultrafast -b:v 2000k \
    -c:a aac -b:a 128k \
    test_pattern.mp4 2>&1 | grep -E "(Duration|time=)" || true

# Generate a color bars video (5 seconds)
echo "[2/4] Generating color bars video (color_bars.mp4)..."
ffmpeg -y -f lavfi -i smptebars=duration=5:size=1920x1080:rate=25 \
    -f lavfi -i sine=frequency=440:duration=5 \
    -c:v libx264 -preset ultrafast -b:v 3000k \
    -c:a aac -b:a 128k \
    color_bars.mp4 2>&1 | grep -E "(Duration|time=)" || true

# Generate a counting video (15 seconds)
echo "[3/4] Generating counting video (counting.mp4)..."
ffmpeg -y -f lavfi -i "color=c=blue:s=640x480:d=15" \
    -vf "drawtext=fontfile=/System/Library/Fonts/Helvetica.ttc:fontsize=120:fontcolor=white:x=(w-text_w)/2:y=(h-text_h)/2:text='%{eif\:t\:d}'" \
    -f lavfi -i sine=frequency=800:duration=15 \
    -c:v libx264 -preset ultrafast -b:v 1000k \
    -c:a aac -b:a 128k \
    counting.mp4 2>&1 | grep -E "(Duration|time=)" || true

# Generate a small test file for upload testing
echo "[4/4] Generating small test file (small_test.txt)..."
cat > small_test.txt << 'INNER_EOF'
This is a test file for upload/download testing.
Generated at: $(date)
File size: Small (~100 bytes)
Purpose: Testing file browser upload/download functionality
INNER_EOF

echo ""
echo "✓ Test media generation complete!"
echo ""
echo "Generated files:"
ls -lh "$MEDIA_DIR"
echo ""
echo "Total size:"
du -sh "$MEDIA_DIR"
