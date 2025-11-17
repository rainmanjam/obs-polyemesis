# OBS Polyemesis - Restreamer Integration Test Runner (Windows PowerShell)
# Orchestrates Docker setup, test execution, and cleanup

param(
    [switch]$KeepContainer,
    [switch]$NoCleanup,
    [int]$Port = 8080,
    [switch]$Help
)

# Configuration
$DOCKER_CONTAINER = "restreamer-test"
$RESTREAMER_PORT = $Port
$RTMP_PORT = 1935
$RESTREAMER_IMAGE = if ($env:RESTREAMER_IMAGE) { $env:RESTREAMER_IMAGE } else { "datarhei/restreamer:latest" }
$RESTREAMER_URL = "http://localhost:$RESTREAMER_PORT"
$RESTREAMER_USER = if ($env:RESTREAMER_USER) { $env:RESTREAMER_USER } else { "admin" }
$RESTREAMER_PASS = if ($env:RESTREAMER_PASS) { $env:RESTREAMER_PASS } else { "admin" }
$CLEANUP_ON_EXIT = -not $NoCleanup

# Colors
function Write-Info { param($msg) Write-Host "[INFO] $msg" -ForegroundColor Blue }
function Write-Success { param($msg) Write-Host "[SUCCESS] $msg" -ForegroundColor Green }
function Write-Error { param($msg) Write-Host "[ERROR] $msg" -ForegroundColor Red }
function Write-Warning { param($msg) Write-Host "[WARNING] $msg" -ForegroundColor Yellow }
function Write-Section {
    param($msg)
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host " $msg" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
}

# Show help
if ($Help) {
    Write-Host "Usage: .\run-integration-tests.ps1 [OPTIONS]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -KeepContainer  Keep Docker container running after tests"
    Write-Host "  -NoCleanup      Don't cleanup Docker resources"
    Write-Host "  -Port PORT      Use custom port (default: 8080)"
    Write-Host "  -Help           Show this help message"
    exit 0
}

# Cleanup function
function Cleanup {
    if ($CLEANUP_ON_EXIT -and -not $KeepContainer) {
        Write-Section "Cleanup"

        $running = docker ps -q -f name="$DOCKER_CONTAINER" 2>$null
        if ($running) {
            Write-Info "Stopping Docker container..."
            docker stop "$DOCKER_CONTAINER" 2>&1 | Out-Null
        }

        $exists = docker ps -aq -f name="$DOCKER_CONTAINER" 2>$null
        if ($exists) {
            Write-Info "Removing Docker container..."
            docker rm "$DOCKER_CONTAINER" 2>&1 | Out-Null
        }

        Write-Success "Cleanup complete"
    } elseif ($KeepContainer) {
        Write-Warning "Container '$DOCKER_CONTAINER' is still running"
        Write-Info "Connect to Restreamer at: $RESTREAMER_URL"
        Write-Info "Stop with: docker stop $DOCKER_CONTAINER"
    }
}

# Register cleanup on exit
Register-EngineEvent PowerShell.Exiting -Action { Cleanup } | Out-Null

# Main execution
try {
    Write-Section "OBS Polyemesis - Restreamer Integration Tests"

    # Check Docker
    Write-Info "Checking Docker..."
    $dockerVersion = docker --version 2>$null
    if (-not $dockerVersion) {
        Write-Error "Docker not found. Please install Docker Desktop first."
        exit 1
    }
    Write-Success "Docker found: $dockerVersion"

    # Check Python
    Write-Info "Checking Python..."
    $pythonVersion = py --version 2>$null
    if (-not $pythonVersion) {
        Write-Error "Python 3 not found. Please install Python 3.8+"
        exit 1
    }
    Write-Success "Python found: $pythonVersion"

    # Check Python dependencies
    Write-Info "Checking Python dependencies..."
    $hasRequests = py -c "import requests" 2>$null
    if (-not $?) {
        Write-Warning "requests module not found"
        Write-Info "Installing Python dependencies..."
        py -m pip install requests --quiet
    }
    Write-Success "Python dependencies ready"

    # Docker Setup
    Write-Section "Docker Setup"

    # Stop existing container
    $running = docker ps -q -f name="$DOCKER_CONTAINER" 2>$null
    if ($running) {
        Write-Warning "Existing container found, stopping..."
        docker stop "$DOCKER_CONTAINER" 2>&1 | Out-Null
        docker rm "$DOCKER_CONTAINER" 2>&1 | Out-Null
    }

    # Pull latest image
    Write-Info "Pulling Restreamer image: $RESTREAMER_IMAGE"
    docker pull "$RESTREAMER_IMAGE" 2>&1 | Out-Null
    if (-not $?) {
        Write-Warning "Failed to pull latest image, using cached version"
    }

    # Start Restreamer
    Write-Info "Starting Restreamer container..."
    docker run -d `
        --name "$DOCKER_CONTAINER" `
        -p "${RESTREAMER_PORT}:8080" `
        -p "${RTMP_PORT}:1935" `
        -e "RESTREAMER_USERNAME=$RESTREAMER_USER" `
        -e "RESTREAMER_PASSWORD=$RESTREAMER_PASS" `
        "$RESTREAMER_IMAGE" 2>&1 | Out-Null

    if (-not $?) {
        Write-Error "Failed to start Restreamer container"
        exit 1
    }

    Write-Success "Restreamer container started"

    # Wait for Restreamer to be ready
    Write-Info "Waiting for Restreamer to be ready..."
    $MAX_WAIT = 60
    $WAIT_COUNT = 0

    while ($WAIT_COUNT -lt $MAX_WAIT) {
        try {
            $auth = [System.Convert]::ToBase64String([System.Text.Encoding]::ASCII.GetBytes("${RESTREAMER_USER}:${RESTREAMER_PASS}"))
            $headers = @{ Authorization = "Basic $auth" }
            $response = Invoke-WebRequest -Uri "$RESTREAMER_URL/api/v3/process" -Headers $headers -UseBasicParsing -ErrorAction Stop
            Write-Success "Restreamer is ready!"
            break
        } catch {
            Write-Host "." -NoNewline
            Start-Sleep -Seconds 2
            $WAIT_COUNT += 2
        }
    }

    if ($WAIT_COUNT -ge $MAX_WAIT) {
        Write-Error "Restreamer failed to start within $MAX_WAIT seconds"
        Write-Info "Container logs:"
        docker logs "$DOCKER_CONTAINER" 2>&1 | Select-Object -Last 20
        exit 1
    }

    Write-Host ""

    # Verify Restreamer API
    Write-Info "Verifying Restreamer API..."
    Write-Success "API responding: OK"

    # Run integration tests
    Write-Section "Running Integration Tests"

    $env:RESTREAMER_URL = $RESTREAMER_URL
    $env:RESTREAMER_USER = $RESTREAMER_USER
    $env:RESTREAMER_PASS = $RESTREAMER_PASS
    $env:PYTHONIOENCODING = "utf-8"

    Write-Info "Test suite: Comprehensive Restreamer Integration"
    Write-Info "Test file: test-restreamer-integration.py"
    Write-Host ""

    py test-restreamer-integration.py
    $TEST_EXIT_CODE = $LASTEXITCODE

    Write-Host ""

    # Test results
    if ($TEST_EXIT_CODE -eq 0) {
        Write-Section "Test Results"
        Write-Success "All integration tests passed!"

        if ($KeepContainer) {
            Write-Info "Restreamer is still running at: $RESTREAMER_URL"
            Write-Info "Username: $RESTREAMER_USER"
            Write-Info "Password: $RESTREAMER_PASS"
        }

        exit 0
    } else {
        Write-Section "Test Results"
        Write-Error "Some integration tests failed"

        Write-Info "Container logs (last 30 lines):"
        docker logs "$DOCKER_CONTAINER" 2>&1 | Select-Object -Last 30

        exit $TEST_EXIT_CODE
    }
} finally {
    Cleanup
}
