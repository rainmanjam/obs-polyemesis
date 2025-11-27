#!/bin/bash
# Docker wrapper for user journey tests
# Bypasses Docker credential store issues on Windows

set -e

export DOCKER_CONFIG=/tmp/docker-config
mkdir -p "$DOCKER_CONFIG"
echo '{"credsStore":""}' > "$DOCKER_CONFIG/config.json"

# Pull image if not exists
if ! docker images ubuntu:22.04 | grep -q ubuntu; then
    echo "Pulling Ubuntu 22.04 image..."
    docker pull ubuntu:22.04
fi

# Create a temporary script to run in container
TEMP_SCRIPT=$(mktemp)
cat > "$TEMP_SCRIPT" << 'EOFSCRIPT'
#!/bin/bash
set -e
apt-get update -qq
apt-get install -y -qq curl jq
/bin/bash /workspace/tests/test-user-journey.sh
EOFSCRIPT

chmod +x "$TEMP_SCRIPT"

# Run tests in Docker container
docker run --rm \
  -v "$(pwd):/workspace" \
  -v "$(pwd)/.secrets:/workspace/.secrets:ro" \
  -v "$TEMP_SCRIPT:/tmp/run-test.sh:ro" \
  -w /workspace \
  ubuntu:22.04 /tmp/run-test.sh

# Cleanup
rm -f "$TEMP_SCRIPT"
