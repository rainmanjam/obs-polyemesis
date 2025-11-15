# OBS Polyemesis - API Integration Tests

This directory contains comprehensive API integration tests for OBS Polyemesis against a live Restreamer instance.

## Overview

The test suite validates all 4 newly implemented features:

- **Feature #7**: Metadata Storage - Process notes, tags, and custom metadata
- **Feature #8**: File System Browser - Browse, upload, download, delete files
- **Feature #9**: Dynamic Output Management - Add/remove streaming destinations
- **Feature #10**: Playout Management - Input source control and reconnection

## Prerequisites

### Local Testing

1. **Docker & Docker Compose**
   ```bash
   # macOS
   brew install docker docker-compose
   
   # Ubuntu
   sudo apt install docker.io docker-compose
   ```

2. **FFmpeg** (for generating test media)
   ```bash
   # macOS
   brew install ffmpeg
   
   # Ubuntu
   sudo apt install ffmpeg
   ```

3. **jq** (JSON processing)
   ```bash
   # macOS
   brew install jq
   
   # Ubuntu
   sudo apt install jq
   ```

4. **act** (for local GitHub Actions testing)
   ```bash
   # macOS
   brew install act
   
   # Ubuntu
   curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash
   ```

## Running Tests

### 1. Quick Start - Full Test Suite

```bash
# From repository root
cd /path/to/obs-polyemesis

# Start Restreamer test environment
docker-compose -f docker-compose.test.yml up -d

# Wait for Restreamer to be ready (30-60 seconds)
# Watch logs:
docker-compose -f docker-compose.test.yml logs -f restreamer

# Run tests
./tests/api_integration_test.sh

# Cleanup
docker-compose -f docker-compose.test.yml down -v
```

### 2. Generate Test Media (Optional)

```bash
cd tests/fixtures
./generate_test_media.sh
```

This creates test video files in `tests/fixtures/media/`:
- `test_pattern.mp4` - 10s test pattern (1280x720)
- `color_bars.mp4` - 5s SMPTE color bars (1920x1080)
- `counting.mp4` - 15s counting video (640x480)
- `small_test.txt` - Text file for upload testing

### 3. Run with Custom Configuration

```bash
# Set environment variables
export RESTREAMER_HOST=localhost
export RESTREAMER_PORT=8080
export RESTREAMER_USER=admin
export RESTREAMER_PASS=datarhei

# Run tests
./tests/api_integration_test.sh
```

### 4. Local GitHub Actions Testing with act

```bash
# Test API integration job
act -j api-integration-tests

# Test build job (Ubuntu)
act -j build-and-test --matrix os:ubuntu-latest

# Test build job (macOS)
act -j build-and-test --matrix os:macos-latest

# Run all workflows
act
```

**Note**: act runs in Docker, so it works on any platform even when testing macOS builds.

## Test Environment

### Docker Services

The `docker-compose.test.yml` configures:

1. **restreamer** (datarhei/restreamer:latest)
   - Port 8080: HTTP API and UI
   - Port 8181: HTTPS (optional)
   - Port 1935: RTMP
   - Port 6000: SRT (UDP)
   - Credentials: admin / datarhei
   - JWT secret: test-secret-key-for-jwt-tokens

2. **nginx-media** (nginx:alpine)
   - Port 8888: Static media server
   - Serves files from `tests/fixtures/media/`

### Health Checks

Both services have health checks configured:
- Restreamer: `curl http://localhost:8080/api/v3/about`
- nginx-media: `wget http://localhost/`

Wait for both to be healthy before running tests.

## Test Script Details

### api_integration_test.sh

**Test Categories:**

1. **Authentication**
   - JWT login
   - Access token validation

2. **Process Management**
   - List processes
   - Create test process
   - Get process details
   - Delete process

3. **Metadata Storage** (Feature #7)
   - Set process metadata (notes, tags, description)
   - Get process metadata
   - Update metadata
   - Multiple metadata fields

4. **File System Browser** (Feature #8)
   - List filesystems
   - List files with filters
   - Upload file
   - Download file
   - Verify file integrity
   - Delete file

5. **Dynamic Output Management** (Feature #9)
   - List current outputs
   - Add new output
   - Verify output added
   - Remove output

6. **Playout Management** (Feature #10)
   - Get playout status
   - Reopen input connection
   - Monitor input state

7. **Extended API**
   - Process state monitoring
   - Order tracking

**Output Format:**

```
[INFO] Starting OBS Polyemesis API Integration Tests
[INFO] Testing against: http://localhost:8080/api/v3

=== Test #1: Authentication - Login
[INFO] ✓ PASS: Authentication successful, got access token

=== Test #2: Process Management - List Processes
[INFO] ✓ PASS: Got process list (3 processes)

...

====================================
Test Summary
====================================
Total Tests:  25
Passed:       25
Failed:       0
====================================
All tests passed!
```

**Exit Codes:**
- `0`: All tests passed
- `1`: One or more tests failed

## CI/CD Integration

### GitHub Actions

The `.github/workflows/test.yml` workflow runs automatically on:
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop`
- Manual trigger via `workflow_dispatch`

**Jobs:**

1. **api-integration-tests** (Ubuntu)
   - Starts Restreamer via Docker Compose
   - Runs full API integration test suite
   - Collects logs on failure
   - Uploads test results as artifacts

2. **build-and-test** (Ubuntu + macOS matrix)
   - Builds plugin on multiple platforms
   - Runs unit tests (Linux only)
   - Uploads build artifacts

**Artifacts:**
- Test results (7 day retention)
- Build outputs (7 day retention)
- Logs on failure

### act Configuration

The `.actrc` file configures act for local testing:
- Uses `catthehacker/ubuntu:act-latest` image
- Enables verbose output
- Binds Docker socket for docker-compose support
- Sets Restreamer environment variables

## Troubleshooting

### Restreamer Not Starting

```bash
# Check logs
docker-compose -f docker-compose.test.yml logs restreamer

# Check health status
docker ps

# Restart services
docker-compose -f docker-compose.test.yml restart
```

### Authentication Failures

```bash
# Verify credentials
curl -X POST http://localhost:8080/api/v3/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"datarhei"}'

# Check JWT secret is set
docker-compose -f docker-compose.test.yml exec restreamer \
  env | grep JWT_SECRET
```

### Port Conflicts

If ports 8080, 8888, 1935, or 6000 are already in use:

```bash
# Edit docker-compose.test.yml and change port mappings
# Example: "8081:8080" instead of "8080:8080"

# Then update RESTREAMER_PORT environment variable
export RESTREAMER_PORT=8081
```

### Test Failures

```bash
# Run tests with verbose curl output
export CURL_VERBOSE=1
./tests/api_integration_test.sh

# Check Restreamer API is responding
curl -v http://localhost:8080/api/v3/about

# Verify test process was created
curl -X GET http://localhost:8080/api/v3/process \
  -H "Authorization: Bearer <token>"
```

## Development

### Adding New Tests

1. Add test function to `api_integration_test.sh`:
   ```bash
   test_new_feature() {
       test_start "New Feature - Description"
       
       RESPONSE=$(curl -s -X GET "${BASE_URL}/endpoint" \
           -H "Authorization: Bearer ${ACCESS_TOKEN}")
       
       if [ condition ]; then
           test_pass "Test passed"
           return 0
       else
           test_fail "Test failed: $RESPONSE"
           return 1
       fi
   }
   ```

2. Call test function in `main()`:
   ```bash
   test_process_list
   create_test_process
   test_new_feature  # Add here
   ```

3. Update test count expectations in this README

### Updating Test Environment

Edit `docker-compose.test.yml` to:
- Change Restreamer version
- Modify environment variables
- Add new services
- Adjust resource limits

## Best Practices

1. **Always cleanup**: Use `docker-compose down -v` to remove volumes
2. **Check logs**: Review Restreamer logs if tests fail
3. **Isolate tests**: Each test should be independent
4. **Use unique IDs**: Test process IDs include `$$` (PID) to avoid conflicts
5. **Validate responses**: Always check API responses with `jq`

## References

- [Restreamer Documentation](https://docs.datarhei.com/)
- [datarhei Core API v3](https://datarhei.github.io/core/)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [act - Local GitHub Actions](https://github.com/nektos/act)
- [Docker Compose Documentation](https://docs.docker.com/compose/)

## License

Same as OBS Polyemesis - GPL-2.0
