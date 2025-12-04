# Integration Tests

This directory contains integration tests that verify multiple modules work together correctly. Tests can run in both mock mode (for CI/CD) and live mode (against a real server).

## Test Files

- **test_restreamer_api_integration.c** - Comprehensive integration tests for Restreamer API v3
- **test_channel_api_integration.c** - Integration tests for Channel/API/Multistream interactions (mock + live modes)

## Tests Included

### test_restreamer_api_integration.c (9 tests)

Tests basic API functionality against live server:

1. **test_api_login** - Tests login and token retrieval using username/password authentication
2. **test_api_token_refresh** - Tests the token refresh mechanism using refresh tokens
3. **test_api_list_processes** - Tests listing all processes from the Restreamer server
4. **test_api_create_process** - Tests creating a new process with correct JSON structure
5. **test_api_process_command** - Tests start/stop commands using PUT method
6. **test_api_delete_process** - Tests deleting a process from the server
7. **test_api_error_handling** - Tests 401 (Unauthorized) and 404 (Not Found) error responses
8. **test_api_invalid_credentials** - Tests authentication failure with wrong credentials
9. **test_process_json_structure** - Verifies JSON structure includes cleanup and limits fields

### test_channel_api_integration.c (12 tests)

Tests module integration (mock mode by default, live mode with LIVE_TEST_SERVER=1):

**Channel to API Integration:**
1. **test_channel_start_creates_api_calls** - Verifies channel start creates correct API calls
2. **test_channel_stop_cleanup** - Verifies channel stop properly cleans up on server
3. **test_channel_state_reflects_server** - Verifies channel state reflects server state

**Multistream to API Integration:**
4. **test_multistream_config_json** - Verifies multistream config creates correct process JSON
5. **test_output_url_construction** - Tests output URL construction for different services
6. **test_orientation_video_filtering** - Tests orientation-based video filtering

**Configuration Integration:**
7. **test_config_change_propagation** - Tests config changes propagate to API client
8. **test_reconnection_on_config_change** - Tests reconnection on config change
9. **test_invalid_server_config** - Tests handling of invalid server config

**Error Recovery:**
10. **test_recovery_from_api_errors** - Tests recovery from API errors
11. **test_server_disconnection** - Tests handling of server disconnection (mock only)
12. **test_automatic_reconnection** - Tests automatic reconnection features

## Server Configuration

The tests are configured to run against:

- **Server URL**: https://rs2.rainmanjam.com
- **Port**: 443 (HTTPS)
- **Username**: admin
- **Password**: tenn2jagWEE@##$
- **SSL Verification**: Disabled (-k flag equivalent)

## Building

The integration tests are built automatically with the main test suite:

```bash
cd build
cmake ..
make test_restreamer_api_integration
```

## Running

### Channel-API Integration Tests (Mock + Live Modes)

**Mock Mode (Default - for CI/CD):**
```bash
cd build/tests
./test_channel_api_integration
```

**Live Mode (against real server):**
```bash
cd build/tests
LIVE_TEST_SERVER=1 ./test_channel_api_integration
```

**Using CTest:**
```bash
cd build
# Mock mode
ctest -R channel_api_integration -V

# Live mode
LIVE_TEST_SERVER=1 ctest -R channel_api_integration -V
```

### Restreamer API Integration Tests (Live Server Only)

**Using the executable directly:**
```bash
cd build/tests
./test_restreamer_api_integration
```

**Using CTest:**
```bash
cd build
ctest -R restreamer_api_integration -V
```

### Run All Integration Tests

```bash
cd build
# Run all integration tests (mock mode)
ctest -L integration -V

# Run all integration tests (live mode)
LIVE_TEST_SERVER=1 ctest -L integration -V
```

### Run Only Network Tests

```bash
cd build
ctest -L network -V
```

## Test Labels

The integration tests are tagged with the following CTest labels:

- `integration` - Marks the test as an integration test
- `network` - Indicates the test requires network access
- `live-server` - Indicates the test requires a live Restreamer server

## Requirements

- A running Restreamer server (v3 API)
- Network connectivity to the server
- Valid credentials (username/password)
- libcurl with SSL support
- jansson for JSON parsing

## Expected Results

All tests should pass when run against a properly configured Restreamer server. Some tests may report "expected failures" if certain conditions aren't met (e.g., no input sources available for process creation).

## Troubleshooting

### Connection Failures

If tests fail with connection errors:

1. Verify the server is accessible: `curl -k https://rs2.rainmanjam.com/api/v3/`
2. Check network connectivity
3. Verify SSL certificate issues (tests use -k flag to bypass)
4. Check firewall settings

### Authentication Failures

If tests fail with 401 errors:

1. Verify credentials are correct
2. Check if the server requires token-based auth
3. Verify the API endpoint is correct

### Process Creation Failures

Process creation tests may fail if:

1. No valid input sources are available
2. The server has reached process limits
3. The specified output URLs are invalid

These failures are expected and the tests account for them.

## Customizing Server Configuration

To test against a different server, modify the following constants in `test_restreamer_api_integration.c`:

```c
#define TEST_SERVER_HOST "your-server.com"
#define TEST_SERVER_PORT 443
#define TEST_SERVER_USERNAME "your-username"
#define TEST_SERVER_PASSWORD "your-password"
#define TEST_SERVER_USE_HTTPS true
```

## Security Notes

⚠️ **Warning**: This test file contains hardcoded credentials for testing purposes. In production:

- Never commit credentials to version control
- Use environment variables or secure credential storage
- Implement proper secret management
- Enable SSL verification in production

## Contributing

When adding new integration tests:

1. Follow the existing test pattern using the test framework
2. Add descriptive test names and documentation
3. Handle expected failures gracefully
4. Clean up resources (delete test processes, etc.)
5. Update this README with new test descriptions

## See Also

- [Main Tests README](../README_INTEGRATION_TESTS.md)
- [Plugin Tests Documentation](../README_PLUGIN_TESTS.md)
- [Restreamer API Documentation](../../docs/restreamer-api.md)
