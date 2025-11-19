# OBS Polyemesis - Comprehensive Restreamer Integration Tests

## Overview

This test suite provides comprehensive integration testing for the OBS Polyemesis plugin's Restreamer connectivity. It tests ALL areas of the plugin-Restreamer interaction.

## Test Coverage

The integration tests cover 8 major areas with 27+ individual tests:

### 1. Connection & Authentication (3 tests)
- Initial connection to Restreamer
- Invalid credentials rejection
- API version compatibility
- Connection resilience

### 2. Process Management (5 tests)
- Create new process (CRUD - Create)
- Read process details (CRUD - Read)
- List all processes (CRUD - Read)
- Update process configuration (CRUD - Update)
- Delete process (CRUD - Delete)

### 3. Process Control (4 tests)
- Start streaming process
- Get process state/status
- Stop streaming process
- Restart streaming process

### 4. Multi-Destination Streaming (2 tests)
- Create process with multiple outputs (5+ destinations)
- Simulate streaming to different platforms (Twitch, YouTube, Facebook)

###5. Process Monitoring (4 tests)
- Get detailed process state
- Get process report/metrics
- Get process configuration
- Probe process input

### 6. Metadata Storage (2 tests)
- Set process metadata
- Get process metadata

### 7. Error Handling (4 tests)
- Handle non-existent process (404 errors)
- Prevent duplicate process IDs
- Handle invalid input URLs
- Validate required fields

### 8. Performance & Concurrency (2 tests)
- Multiple concurrent processes (10+ simultaneous)
- Rapid create/delete cycles

## Files

### `test-restreamer-integration.py`
Comprehensive Python test suite using unittest framework. Tests all Restreamer API endpoints and functionality.

**Test Classes:**
- `Test01_ConnectionAuthentication` - Connection and auth tests
- `Test02_ProcessManagement` - CRUD operations
- `Test03_ProcessControl` - Start/stop/restart
- `Test04_MultiDestination` - Multi-platform streaming
- `Test05_ProcessMonitoring` - Monitoring and metrics
- `Test06_Metadata` - Metadata storage
- `Test07_ErrorHandling` - Error scenarios
- `Test08_Performance` - Performance and load testing

### `run-integration-tests.sh`
Test orchestration script that:
1. Checks prerequisites (Docker, Python)
2. Starts Restreamer in Docker
3. Waits for Restreamer to be ready
4. Runs all integration tests
5. Cleans up Docker resources

## Quick Start

### Prerequisites

**Required:**
- Docker installed and running
- Python 3.8 or higher
- `requests` Python module

**Optional:**
- `jq` for JSON formatting (improves output)

### Installation

```bash
# Install Python dependencies
pip3 install requests

# Verify Docker is running
docker ps

# Make scripts executable
chmod +x tests/run-integration-tests.sh tests/test-restreamer-integration.py
```

### Running Tests

**Simple (recommended):**
```bash
cd tests
./run-integration-tests.sh
```

**With options:**
```bash
# Keep Docker container running after tests
./run-integration-tests.sh --keep-container

# Use custom port
./run-integration-tests.sh --port 9090

# Don't cleanup on exit
./run-integration-tests.sh --no-cleanup

# See all options
./run-integration-tests.sh --help
```

**Manual (advanced):**
```bash
# Start Restreamer manually
docker run -d --name restreamer-test \
  -p 8080:8080 -p 1935:1935 \
  -e RESTREAMER_USERNAME=admin \
  -e RESTREAMER_PASSWORD=admin \
  datarhei/restreamer:latest

# Wait for it to be ready
until curl -sf http://localhost:8080/api/v3/process; do sleep 2; done

# Run tests
cd tests
export RESTREAMER_URL=http://localhost:8080
export RESTREAMER_USER=admin
export RESTREAMER_PASS=admin
python3 test-restreamer-integration.py

# Cleanup
docker stop restreamer-test
docker rm restreamer-test
```

## Expected Output

```
========================================
 OBS Polyemesis - Restreamer Integration Tests
========================================

[INFO] Checking Docker...
[SUCCESS] Docker found: Docker version 20.10.x

[INFO] Checking Python...
[SUCCESS] Python found: Python 3.11.x

========================================
 Docker Setup
========================================

[INFO] Starting Restreamer container...
[SUCCESS] Restreamer container started
[INFO] Waiting for Restreamer to be ready...
.[SUCCESS] Restreamer is ready!

========================================
 Running Integration Tests
========================================

test_01_initial_connection ... ok
test_02_invalid_credentials ... ok
test_01_create_process ... ok
test_02_read_process ... ok
...

========================================
RESTREAMER INTEGRATION TEST SUMMARY
========================================
Total tests run: 27
Successes:       26
Failures:        0
Errors:          1

[SUCCESS] All integration tests passed!
```

## Environment Variables

Configure the tests using these environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `RESTREAMER_URL` | `http://localhost:8080` | Restreamer API URL |
| `RESTREAMER_USER` | `admin` | Restreamer username |
| `RESTREAMER_PASS` | `admin` | Restreamer password |
| `RESTREAMER_IMAGE` | `datarhei/restreamer:latest` | Docker image to use |

**Example:**
```bash
export RESTREAMER_URL=http://192.168.1.100:8080
export RESTREAMER_USER=myuser
export RESTREAMER_PASS=mypass
./run-integration-tests.sh
```

## Test Details

### Connection & Authentication Tests

**Test 1.1: Initial connection**
- Verifies connection to Restreamer API
- Checks API responds to `/api/v3/process`
- Confirms authentication works

**Test 1.2: Invalid credentials**
- Tests rejection of bad credentials
- Verifies auth is enforced (or notes if anonymous access allowed)

**Test 1.3: API version**
- Checks API endpoint availability
- Verifies v3 API compatibility

### Process Management Tests

**Test 2.1: Create process**
- Creates a new streaming process
- Verifies process ID and configuration
- Tests basic input/output setup

**Test 2.2: Read process**
- Retrieves process details by ID
- Verifies configuration is returned correctly

**Test 2.3: List processes**
- Lists all processes
- Verifies array response

**Test 2.4: Update process**
- Modifies existing process
- Adds new output destination
- Verifies changes are persisted

**Test 2.5: Delete process**
- Deletes a process
- Verifies deletion (404 on subsequent reads)

### Process Control Tests

**Test 3.1: Start process**
- Sends start command
- Verifies process transitions to running/starting state

**Test 3.2: Get state**
- Retrieves detailed process state
- Checks exec, order fields

**Test 3.3: Stop process**
- Sends stop command
- Verifies process transitions to stopped state

**Test 3.4: Restart process**
- Sends restart command
- Verifies process restarts

### Multi-Destination Tests

**Test 4.1: Multiple outputs**
- Creates process with 5+ outputs
- Verifies all outputs configured

**Test 4.2: Platform simulation**
- Simulates Twitch, YouTube, Facebook outputs
- Tests multi-platform setup

### Monitoring Tests

**Test 5.1: State details**
- Gets detailed process state
- Checks metrics availability

**Test 5.2: Process report**
- Retrieves process metrics/report
- Checks created_at, stats

**Test 5.3: Process config**
- Gets full process configuration
- Verifies reference, ID

**Test 5.4: Input probe**
- Probes input stream (if active)
- Checks codec information

### Metadata Tests

**Test 6.1: Set metadata**
- Sets custom key-value metadata
- Verifies write succeeds

**Test 6.2: Get metadata**
- Retrieves previously set metadata
- Verifies values match

### Error Handling Tests

**Test 7.1: Non-existent process**
- Requests non-existent process
- Verifies 404 response

**Test 7.2: Duplicate process ID**
- Attempts to create duplicate
- Verifies rejection

**Test 7.3: Invalid input URL**
- Creates process with invalid URL
- Process created but will fail on start

**Test 7.4: Missing fields**
- Attempts creation without required fields
- Verifies validation error

### Performance Tests

**Test 8.1: Concurrent processes**
- Creates 10 processes simultaneously
- Verifies all created successfully

**Test 8.2: Rapid operations**
- Performs 5 rapid create/delete cycles
- Tests API resilience

## Troubleshooting

### Tests Fail to Connect

**Problem:** `Connection refused` or `Connection timeout`

**Solutions:**
1. Check Docker is running: `docker ps`
2. Check container is up: `docker ps -f name=restreamer-test`
3. Check logs: `docker logs restreamer-test`
4. Try manual curl: `curl http://localhost:8080/api/v3/process`

### Docker Permission Denied

**Problem:** `permission denied while trying to connect to Docker daemon`

**Solutions:**
1. Add user to docker group: `sudo usermod -aG docker $USER`
2. Re-login or run: `newgrp docker`
3. Or use sudo: `sudo ./run-integration-tests.sh`

### Port Already in Use

**Problem:** `port 8080 already allocated`

**Solutions:**
1. Use different port: `./run-integration-tests.sh --port 9090`
2. Stop conflicting container: `docker stop $(docker ps -q -f publish=8080)`
3. Find what's using port: `lsof -i :8080`

### Tests Pass But Some Show "ℹ" Info

This is normal! The "ℹ" indicates the test adapted to server configuration:
- Anonymous access allowed (some configs don't require auth)
- Endpoint not available (different Restreamer versions)
- Feature requires active stream (probe, report)

These are informational, not failures.

### API Version Mismatch

Different Restreamer versions have slightly different API responses. The tests handle this gracefully. Common variations:

- datarhei-core v16.x: Uses `exec` field for state
- Restreamer 2.x: Different state format
- Some fields optional in different versions

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Restreamer Integration Tests

on: [push, pull_request]

jobs:
  integration-tests:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.11'

      - name: Install dependencies
        run: pip install requests

      - name: Run integration tests
        run: |
          cd tests
          ./run-integration-tests.sh
```

### GitLab CI Example

```yaml
integration-tests:
  image: docker:latest
  services:
    - docker:dind
  before_script:
    - apk add --no-cache python3 py3-pip
    - pip3 install requests
  script:
    - cd tests
    - ./run-integration-tests.sh
```

## Extending Tests

### Adding New Tests

1. **Add test to appropriate class:**

```python
class Test02_ProcessManagement(unittest.TestCase):
    def test_06_my_new_test(self):
        """Test 2.6: My new functionality"""
        # Your test code here
        result = self.client.get('/api/v3/some/endpoint')
        self.assertEqual(result['status'], 'ok')
        print("✓ My new test passed")
```

2. **Run tests:**
```bash
python3 test-restreamer-integration.py
```

### Adding New Test Class

```python
class Test09_MyNewFeature(unittest.TestCase):
    """Test 9: My new feature area"""

    @classmethod
    def setUpClass(cls):
        cls.client = RestreamerAPIClient(
            os.getenv('RESTREAMER_URL', 'http://localhost:8080'),
            os.getenv('RESTREAMER_USER', 'admin'),
            os.getenv('RESTREAMER_PASS', 'admin')
        )

    def test_01_feature_test(self):
        """Test 9.1: Feature description"""
        # Test code
        pass
```

Then add to `run_tests()`:
```python
test_classes = [
    # ... existing classes ...
    Test09_MyNewFeature,
]
```

## Best Practices

1. **Always use test-specific IDs:** Include timestamp in process IDs to avoid collisions
2. **Clean up resources:** Use `tearDownClass` to delete test processes
3. **Handle API variations:** Use try/except for features that may not be available
4. **Descriptive test names:** Follow pattern `test_XX_description` where XX is number
5. **Print informative messages:** Use `print()` to show what's being tested

## Performance Metrics

Expected test execution times:

- Connection tests: ~1-2 seconds
- Process management: ~5-10 seconds
- Process control: ~10-15 seconds (includes wait times)
- Multi-destination: ~3-5 seconds
- Monitoring: ~2-4 seconds
- Metadata: ~1-2 seconds
- Error handling: ~3-5 seconds
- Performance: ~15-20 seconds (creates/deletes many processes)

**Total: ~45-60 seconds**

## Support

For issues with these tests:

1. Check this README for troubleshooting
2. Review test output for specific error messages
3. Check Restreamer logs: `docker logs restreamer-test`
4. Open an issue on GitHub with:
   - Test output
   - Docker version
   - Python version
   - Restreamer version
   - OS/platform

## Related Documentation

- [Plugin Testing](README_PLUGIN_TESTS.md) - OBS plugin loading/UI tests
- [API Reference](../src/restreamer-api.h) - Full API documentation
- [Restreamer API Docs](https://datarhei.github.io/restreamer/) - Official Restreamer documentation
