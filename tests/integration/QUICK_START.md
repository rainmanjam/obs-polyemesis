# Quick Start - Live Integration Tests

## TL;DR - Run the Tests

```bash
# From project root
./tests/run-live-integration-tests.sh
```

## Or using CMake/CTest

```bash
# Build the tests
cd build
cmake --build . --target test_restreamer_api_integration

# Run the tests
./tests/test_restreamer_api_integration

# Or with CTest
ctest -R restreamer_api_integration -V
```

## What Gets Tested

✅ **Authentication**
- Login with username/password
- Token retrieval
- Token refresh mechanism

✅ **Process Management**
- List all processes
- Create new process
- Start/stop processes (PUT method)
- Delete processes

✅ **Error Handling**
- 401 Unauthorized responses
- 404 Not Found responses
- Invalid credentials

✅ **JSON Structure**
- Verify cleanup fields
- Verify limits fields
- Validate response format

## Test Server

- **URL**: https://rs2.rainmanjam.com
- **API Version**: v3
- **Transport**: HTTPS (SSL verification disabled)

## Requirements

- libcurl (with SSL support)
- jansson (JSON library)
- Network connectivity
- Running Restreamer server

## Expected Output

```
===============================================================================
TEST SUITE: Integration Tests - Live Restreamer API
===============================================================================

[TEST] Running: Test 1: API login and token retrieval
  Testing login to rs2.rainmanjam.com...
  Response code: 200
  Access token: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
[PASS] Test 1: API login and token retrieval

[TEST] Running: Test 2: API token refresh mechanism
  Testing token refresh mechanism...
[PASS] Test 2: API token refresh mechanism

...

===============================================================================
TEST SUMMARY
===============================================================================
Total:   9
Passed:  9
Failed:  0
Crashed: 0
Skipped: 0
===============================================================================
Result: PASSED
===============================================================================
```

## Troubleshooting

### Connection Failed

```bash
# Test server connectivity
curl -k https://rs2.rainmanjam.com/api/v3/

# Should return API info or 401 if auth required
```

### Build Failed

```bash
# Reconfigure and rebuild
cd build
cmake ..
cmake --build . --target test_restreamer_api_integration
```

### Tests Timeout

Some tests may take time due to network latency. This is normal.

## Next Steps

After successful tests:

1. Review test output for any warnings
2. Check server logs for any errors
3. Run additional test suites if needed
4. Integrate into CI/CD pipeline

## CI/CD Integration

### GitHub Actions Example

```yaml
- name: Run Integration Tests
  run: |
    cd build
    ctest -L integration -V
  env:
    RESTREAMER_HOST: ${{ secrets.RESTREAMER_HOST }}
    RESTREAMER_USER: ${{ secrets.RESTREAMER_USER }}
    RESTREAMER_PASS: ${{ secrets.RESTREAMER_PASS }}
```

### GitLab CI Example

```yaml
integration_tests:
  stage: test
  script:
    - cd build
    - ctest -L integration -V
  only:
    - main
    - develop
```

## Support

For issues or questions:

1. Check the main README: `../README_INTEGRATION_TESTS.md`
2. Review the test code: `test_restreamer_api_integration.c`
3. Check Restreamer API docs
4. Open an issue on GitHub
