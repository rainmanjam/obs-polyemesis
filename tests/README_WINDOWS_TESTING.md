# OBS Polyemesis - Windows Testing Guide

## Overview

This guide covers running the comprehensive integration tests on Windows. The test suite uses Docker to run a Restreamer instance and Python to execute the tests.

## Prerequisites

### Required Software

1. **Docker Desktop for Windows**
   - Version: 28.5.1 or higher
   - Download: https://www.docker.com/products/docker-desktop/
   - Ensure Docker is running before executing tests

2. **Python**
   - Version: 3.8 or higher (tested with Python 3.13.9)
   - Use the Python launcher (`py`) which is installed by default with Python on Windows
   - Download: https://www.python.org/downloads/

3. **Python Dependencies**
   - `requests` module (automatically installed by test runner if missing)
   - Install manually: `py -m pip install requests`

### Optional Software

- **PowerShell 5.0+** (included with Windows 10/11)
- **Git for Windows** (if syncing repository)

## Test Files

The following files are required in the `tests/` directory:

1. **run-integration-tests.ps1** - PowerShell test orchestration script
2. **test-restreamer-integration.py** - Python integration test suite
3. **README_INTEGRATION_TESTS.md** - General integration testing documentation

## Quick Start

### Running Tests on Windows

1. **Open PowerShell** as Administrator or regular user

2. **Navigate to the tests directory:**
   ```powershell
   cd C:\path\to\obs-polyemesis\tests
   ```

3. **Run the test script:**
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\run-integration-tests.ps1
   ```

### Command Options

```powershell
# Show help
.\run-integration-tests.ps1 -Help

# Keep Docker container running after tests
.\run-integration-tests.ps1 -KeepContainer

# Use custom port
.\run-integration-tests.ps1 -Port 9090

# Don't cleanup Docker resources
.\run-integration-tests.ps1 -NoCleanup
```

## Expected Output

```
========================================
 OBS Polyemesis - Restreamer Integration Tests
========================================

[INFO] Checking Docker...
[SUCCESS] Docker found: Docker version 28.5.1, build e180ab8
[INFO] Checking Python...
[SUCCESS] Python found: Python 3.13.9
[INFO] Checking Python dependencies...
[SUCCESS] Python dependencies ready

========================================
 Docker Setup
========================================

[INFO] Pulling Restreamer image: datarhei/restreamer:latest
[INFO] Starting Restreamer container...
[SUCCESS] Restreamer container started
[INFO] Waiting for Restreamer to be ready...
.[SUCCESS] Restreamer is ready!

========================================
 Running Integration Tests
========================================

[INFO] Test suite: Comprehensive Restreamer Integration
[INFO] Test file: test-restreamer-integration.py

✓ Connected to Restreamer (found 0 processes)
[... 26 tests ...]

================================================================================
RESTREAMER INTEGRATION TEST SUMMARY
================================================================================
Total tests run: 26
Successes:       26
Failures:        0
Errors:          0
================================================================================

[SUCCESS] All integration tests passed!
```

## Test Execution Time

Expected test completion time on Windows:
- Docker setup: ~30-60 seconds (first run, includes image pull)
- Test execution: ~6-10 seconds
- Cleanup: ~2-5 seconds
- **Total: ~40-75 seconds**

## Running Tests via SSH (Remote Windows Machine)

If you're running tests on a remote Windows machine via SSH:

```bash
# From macOS/Linux machine
ssh windows-host "cd C:\\workspace\\obs-polyemesis\\tests && powershell -ExecutionPolicy Bypass -File .\\run-integration-tests.ps1"
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
```powershell
$env:RESTREAMER_URL = "http://192.168.1.100:8080"
$env:RESTREAMER_USER = "myuser"
$env:RESTREAMER_PASS = "mypass"
.\run-integration-tests.ps1
```

## Troubleshooting

### Docker Desktop Not Running

**Problem:** Error connecting to Docker daemon

**Solutions:**
1. Start Docker Desktop from the Start Menu
2. Wait for Docker Desktop to fully initialize (whale icon in system tray)
3. Verify Docker is running: `docker ps`

### PowerShell Execution Policy

**Problem:** `execution of scripts is disabled on this system`

**Solutions:**
```powershell
# Option 1: Bypass policy for single execution
powershell -ExecutionPolicy Bypass -File .\run-integration-tests.ps1

# Option 2: Change policy (requires Administrator)
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### Python Not Found

**Problem:** `py : The term 'py' is not recognized`

**Solutions:**
1. Install Python from https://www.python.org/downloads/
2. During installation, check "Add Python to PATH"
3. Restart PowerShell after installation
4. Verify: `py --version`

### Port Already in Use

**Problem:** `port 8080 already allocated`

**Solutions:**
```powershell
# Option 1: Use different port
.\run-integration-tests.ps1 -Port 9090

# Option 2: Stop conflicting container
docker stop $(docker ps -q -f publish=8080)

# Option 3: Find what's using the port
netstat -ano | findstr :8080
```

### Unicode/Encoding Errors

**Problem:** `UnicodeEncodeError: 'charmap' codec can't encode character`

**Solution:**
The test runner script automatically sets `PYTHONIOENCODING=utf-8`. If you still encounter issues:
```powershell
$env:PYTHONIOENCODING = "utf-8"
py test-restreamer-integration.py
```

### Docker Container Won't Start

**Problem:** Container starts but Restreamer doesn't become ready

**Solutions:**
1. Check Docker logs:
   ```powershell
   docker logs restreamer-test
   ```

2. Verify Docker has sufficient resources:
   - Docker Desktop → Settings → Resources
   - Minimum: 2 GB RAM, 2 CPUs

3. Try pulling image manually:
   ```powershell
   docker pull datarhei/restreamer:latest
   ```

### Tests Hang or Timeout

**Problem:** Tests appear to freeze or timeout

**Solutions:**
1. Check Docker container status:
   ```powershell
   docker ps -a
   docker logs restreamer-test
   ```

2. Increase timeout in PowerShell script (edit `run-integration-tests.ps1`):
   ```powershell
   $MAX_WAIT = 120  # Increase from 60 to 120 seconds
   ```

3. Kill and restart:
   ```powershell
   docker stop restreamer-test
   docker rm restreamer-test
   .\run-integration-tests.ps1
   ```

## Windows-Specific Features

### PowerShell Script Features

The Windows PowerShell script includes:

1. **Automatic dependency checking**
   - Verifies Docker is installed and running
   - Checks Python and py launcher availability
   - Installs missing Python modules automatically

2. **Color-coded output**
   - Blue: Informational messages
   - Green: Success messages
   - Red: Error messages
   - Yellow: Warning messages
   - Cyan: Section headers

3. **Automatic cleanup**
   - Stops and removes test containers on exit
   - Handles Ctrl+C gracefully
   - Optional container persistence with `-KeepContainer`

4. **UTF-8 encoding support**
   - Automatically sets `PYTHONIOENCODING=utf-8`
   - Ensures Unicode characters display correctly
   - Handles Windows console encoding differences

## File Syncing (for Remote Testing)

If testing on a remote Windows machine, sync files using:

### Using SCP (from macOS/Linux)
```bash
# Create directory
ssh windows-host "mkdir C:\\workspace\\obs-polyemesis\\tests"

# Copy files
scp tests/test-restreamer-integration.py windows-host:C:/workspace/obs-polyemesis/tests/
scp tests/run-integration-tests.ps1 windows-host:C:/workspace/obs-polyemesis/tests/
scp tests/README_INTEGRATION_TESTS.md windows-host:C:/workspace/obs-polyemesis/tests/
```

### Using Git (on Windows)
```powershell
cd C:\workspace
git clone https://github.com/yourusername/obs-polyemesis.git
cd obs-polyemesis\tests
.\run-integration-tests.ps1
```

## CI/CD Integration

### GitHub Actions (Windows Runner)

```yaml
name: Windows Integration Tests

on: [push, pull_request]

jobs:
  integration-tests-windows:
    runs-on: windows-latest

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
          powershell -ExecutionPolicy Bypass -File .\run-integration-tests.ps1
```

## Performance Metrics

Test execution times on Windows (measured on typical hardware):

- **Connection tests:** ~1-2 seconds
- **Process management:** ~1-2 seconds
- **Process control:** ~1-2 seconds
- **Multi-destination:** ~0.5-1 seconds
- **Monitoring:** ~0.5-1 seconds
- **Metadata:** ~0.5-1 seconds
- **Error handling:** ~0.5-1 seconds
- **Performance tests:** ~1-2 seconds

**Total test execution: ~6-10 seconds**
(Plus ~30-60 seconds for Docker setup on first run)

## Test Coverage

All 26 tests cover:

1. **Connection & Authentication** (3 tests)
   - Initial connection
   - Invalid credentials
   - API endpoints

2. **Process Management** (5 tests)
   - Create, Read, Update, Delete
   - List processes

3. **Process Control** (4 tests)
   - Start, Stop, Restart
   - Get state

4. **Multi-Destination** (2 tests)
   - Multiple outputs
   - Platform simulation

5. **Monitoring** (4 tests)
   - State details
   - Process report
   - Configuration
   - Input probe

6. **Metadata** (2 tests)
   - Set metadata
   - Get metadata

7. **Error Handling** (4 tests)
   - Non-existent process
   - Duplicate IDs
   - Invalid URLs
   - Missing fields

8. **Performance** (2 tests)
   - Concurrent processes
   - Rapid operations

## Comparison: Windows vs Unix

| Feature | Windows | macOS/Linux |
|---------|---------|-------------|
| **Script Language** | PowerShell (.ps1) | Bash (.sh) |
| **Python Launcher** | `py` | `python3` |
| **Path Separator** | `\` | `/` |
| **Encoding** | UTF-8 (forced) | UTF-8 (default) |
| **Docker** | Docker Desktop | Docker Desktop / Native |
| **Performance** | ~6-10 seconds | ~5-8 seconds |
| **Test Success Rate** | 26/26 (100%) | 26/26 (100%) |

## Best Practices

1. **Always use PowerShell** (not Command Prompt)
2. **Use `py` launcher** instead of `python` or `python3`
3. **Run Docker Desktop before tests**
4. **Use `-ExecutionPolicy Bypass`** to avoid policy issues
5. **Check antivirus software** doesn't block Docker/Python
6. **Ensure adequate disk space** for Docker images (~500MB)

## Support

For Windows-specific issues:

1. Check this README for troubleshooting
2. Review PowerShell script output for errors
3. Check Docker Desktop logs
4. Verify Python and dependencies are installed
5. Open an issue on GitHub with:
   - Windows version
   - PowerShell version (`$PSVersionTable`)
   - Docker version (`docker --version`)
   - Python version (`py --version`)
   - Full test output

## Related Documentation

- [Integration Tests README](README_INTEGRATION_TESTS.md) - General integration testing documentation
- [Plugin Tests README](README_PLUGIN_TESTS.md) - Plugin-specific automated tests
- [Restreamer API Docs](https://datarhei.github.io/restreamer/) - Official Restreamer documentation
