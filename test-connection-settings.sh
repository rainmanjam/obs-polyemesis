#!/bin/bash
# Test script to verify connection settings save/load

set -e

CONFIG_DIR="$HOME/Library/Application Support/obs-studio/plugin_config/obs-polyemesis"
CONFIG_FILE="$CONFIG_DIR/config.json"
BACKUP_FILE="$CONFIG_FILE.test-backup"

echo "=== OBS Polyemesis Connection Settings Test ==="
echo ""

# Backup existing config if it exists
if [ -f "$CONFIG_FILE" ]; then
    echo "Backing up existing config to: $BACKUP_FILE"
    cp "$CONFIG_FILE" "$BACKUP_FILE"
fi

# Create test config with proper keys
echo "Creating test config file..."
mkdir -p "$CONFIG_DIR"

cat > "$CONFIG_FILE" << 'EOF'
{
  "host": "localhost",
  "port": 8080,
  "use_https": false,
  "username": "admin",
  "password": "testpass123"
}
EOF

echo "✓ Test config created at: $CONFIG_FILE"
echo ""
echo "Config contents:"
cat "$CONFIG_FILE" | python3 -m json.tool
echo ""

# Verify the config is valid JSON
if python3 -c "import json; json.load(open('$CONFIG_FILE'))" 2>/dev/null; then
    echo "✓ Config file is valid JSON"
else
    echo "✗ Config file is NOT valid JSON"
    exit 1
fi

echo ""
echo "Expected behavior when OBS loads:"
echo "  1. Dock should load settings and call restreamer_config_load()"
echo "  2. Settings should be parsed: host=localhost, port=8080, use_https=false"
echo "  3. Connection status should attempt to connect"
echo "  4. If no server running, should show 'Disconnected' (red)"
echo ""

echo "Expected behavior when opening Configure dialog:"
echo "  1. Dialog should reconstruct URL as: http://localhost:8080"
echo "  2. Username should show: admin"
echo "  3. Password should show: testpass123"
echo ""

# Restore backup if it exists
if [ -f "$BACKUP_FILE" ]; then
    echo "Test complete. Restore backup with:"
    echo "  mv '$BACKUP_FILE' '$CONFIG_FILE'"
else
    echo "Test complete. Remove test config with:"
    echo "  rm '$CONFIG_FILE'"
fi

echo ""
echo "=== Test Setup Complete ==="
echo "Now test manually by:"
echo "  1. Opening OBS Studio"
echo "  2. Opening Restreamer Control dock"
echo "  3. Checking connection status shows 'Disconnected' (red)"
echo "  4. Opening Configure dialog"
echo "  5. Verifying URL shows 'http://localhost:8080'"
echo "  6. Verifying username/password are populated"
