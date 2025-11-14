# OBS Polyemesis - Automated Testing

## Overview

Automated test suite for OBS Polyemesis plugin, covering API integration, profile management, stream control, and live streaming tests.

## Installation

```bash
# Install Python dependencies
pip3 install -r requirements.txt

# Ensure Restreamer is running
docker run -d --name restreamer-test \
  -p 8080:8080 -p 1935:1935 \
  -e RESTREAMER_USERNAME=admin \
  -e RESTREAMER_PASSWORD=admin \
  datarhei/restreamer:latest
```

## Running Tests

### Quick Start (Recommended)

```bash
# 1. Copy the example environment file
cd tests/automated
cp .env.example .env

# 2. Edit .env and add your credentials
nano .env  # or use your preferred editor

# 3. Run all tests
./run_tests.sh
```

The script automatically loads credentials from `.env` file if present.

### Basic API Tests (No Stream Keys Required)

```bash
# Run all basic tests
python3 test_polyemesis.py

# Run specific test class
python3 -m unittest test_polyemesis.TestProfileManagement

# Run with verbose output
python3 test_polyemesis.py -v
```

### Live Streaming Tests (Stream Keys Required)

**Option 1: Using .env file (Recommended)**

```bash
# Create .env file from example
cp .env.example .env

# Edit .env and add your credentials:
# TWITCH_STREAM_KEY=your_actual_key
# YOUTUBE_STREAM_KEY=your_actual_key

# Run tests (automatically loads .env)
./run_tests.sh
```

**Option 2: Using environment variables**

```bash
# Set up streaming credentials
export TWITCH_INGEST_URL='rtmp://live.twitch.tv/app'
export TWITCH_STREAM_KEY='your_twitch_stream_key'
export YOUTUBE_INGEST_URL='rtmp://a.rtmp.youtube.com/live2'
export YOUTUBE_STREAM_KEY='your_youtube_stream_key'

# Optional: Set OBS RTMP output URL
export OBS_RTMP_URL='rtmp://localhost/live/obs_output'

# Run live streaming tests
python3 test_live_streaming.py
```

### Environment Variables

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `RESTREAMER_URL` | No | `http://localhost:8080` | Restreamer server URL |
| `RESTREAMER_USER` | No | `admin` | Restreamer username |
| `RESTREAMER_PASS` | No | `admin` | Restreamer password |
| `TWITCH_INGEST_URL` | Live tests only | - | Twitch RTMP ingest URL |
| `TWITCH_STREAM_KEY` | Live tests only | - | Your Twitch stream key |
| `YOUTUBE_INGEST_URL` | Live tests only | - | YouTube RTMP ingest URL |
| `YOUTUBE_STREAM_KEY` | Live tests only | - | Your YouTube stream key |
| `OBS_RTMP_URL` | No | `rtmp://localhost/live/obs_output` | OBS RTMP output |

**Security Note:** The `.env` file is automatically excluded from git by `.gitignore`. Never commit your actual stream keys to version control!

## Test Coverage

### Automated Tests ✅

#### Connection & Authentication (test_polyemesis.py)
- ✅ Initial connection to Restreamer
- ✅ Invalid credentials rejection
- ✅ API version compatibility

#### Profile Management (test_polyemesis.py)
- ✅ Create new streaming profile
- ✅ List all profiles
- ✅ Create profile with multiple destinations
- ✅ Delete profile
- ⏸️ Edit profile (pending UI testing)
- ⏸️ Duplicate profile (pending UI testing)

#### Stream Control (test_polyemesis.py)
- ✅ Start streaming process
- ✅ Monitor process state
- ✅ Stop streaming process
- ⏸️ OBS restart behavior (requires OBS integration)

#### Error Handling (test_polyemesis.py)
- ✅ Invalid process ID handling
- ✅ Invalid input URL handling
- ✅ Duplicate process ID rejection
- ⏸️ Network interruption recovery (requires network simulation)
- ⏸️ Server crash recovery (requires orchestration)

#### Performance (test_polyemesis.py)
- ✅ Multiple concurrent profiles
- ✅ Rapid start/stop cycles
- ⏸️ Long-running stream (24h) - requires extended run

#### Live Streaming (test_live_streaming.py)
- ✅ Stream to Twitch
- ✅ Stream to YouTube
- ✅ Simultaneous multistreaming
- ⏸️ Stream quality verification (requires platform APIs)

## Test Results Format

Tests output in standard unittest format:

```
test_01_connection (__main__.TestRestreamerConnection) ... ok
test_02_invalid_credentials (__main__.TestRestreamerConnection) ... ok
test_01_create_profile (__main__.TestProfileManagement) ... ok
...

======================================================================
TEST SUMMARY
======================================================================
Tests run: 18
Successes: 17
Failures: 0
Errors: 1
======================================================================
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Automated Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    services:
      restreamer:
        image: datarhei/restreamer:latest
        ports:
          - 8080:8080
          - 1935:1935
        env:
          RESTREAMER_USERNAME: admin
          RESTREAMER_PASSWORD: admin

    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-python@v4
        with:
          python-version: '3.11'

      - name: Install dependencies
        run: |
          pip install -r tests/automated/requirements.txt

      - name: Wait for Restreamer
        run: |
          timeout 60 bash -c 'until curl -s http://localhost:8080/api/v3/about; do sleep 2; done'

      - name: Run tests
        run: |
          cd tests/automated
          python3 test_polyemesis.py
```

## Troubleshooting

### Connection Refused
**Problem**: `requests.exceptions.ConnectionError: Connection refused`
**Solution**: Ensure Restreamer is running on port 8080

### Authentication Failed
**Problem**: `401 Unauthorized`
**Solution**: Check `RESTREAMER_USER` and `RESTREAMER_PASS` environment variables

### Tests Timeout
**Problem**: Tests hang or timeout
**Solution**: Increase timeout in `RestreamerClient` or check Restreamer logs

### Stream Key Invalid
**Problem**: Live streaming tests fail
**Solution**: Verify stream keys are correct and not expired

## Next Steps

1. Run basic tests to verify setup: `python3 test_polyemesis.py`
2. Configure stream credentials for live tests
3. Run full test suite before releases
4. Integrate with CI/CD pipeline
5. Add custom tests for your specific workflows
