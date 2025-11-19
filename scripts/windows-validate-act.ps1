# Windows Act Validation Script
# Validates GitHub Actions workflows using act on Windows

$ErrorActionPreference = "Continue"

# Colors for output
function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Blue
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

function Write-Error-Msg {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

function Write-Warning-Msg {
    param([string]$Message)
    Write-Host "[WARNING] $Message" -ForegroundColor Yellow
}

# Test results
$script:PASSED = 0
$script:FAILED = 0

function Run-Test {
    param(
        [string]$Name,
        [string]$Command
    )

    Write-Info "Running: $Name"

    try {
        Invoke-Expression $Command
        if ($LASTEXITCODE -eq 0) {
            Write-Success "✓ $Name"
            $script:PASSED++
        } else {
            Write-Error-Msg "✗ $Name (exit code: $LASTEXITCODE)"
            $script:FAILED++
        }
    } catch {
        Write-Error-Msg "✗ $Name (exception: $_)"
        $script:FAILED++
    }
    Write-Host ""
}

Write-Info "OBS Polyemesis - Windows Act Validation"
Write-Host "==========================================" -ForegroundColor Blue
Write-Host ""

# Check if act is installed
if (-not (Get-Command act -ErrorAction SilentlyContinue)) {
    Write-Error-Msg "act is not installed or not in PATH"
    Write-Info "Install act from: https://github.com/nektos/act"
    exit 1
}
Write-Success "act found"
Write-Host ""

# Get project root
$PROJECT_ROOT = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $PROJECT_ROOT

Write-Info "Project root: $PROJECT_ROOT"
Write-Host ""

# 1. Unit Tests (Ubuntu)
Run-Test "Unit Tests (Ubuntu)" `
    "act -j unit-tests-ubuntu -W .github\workflows\run-tests.yaml --pull=false"

# 2. Coverage (Linux)
Run-Test "Coverage (Linux)" `
    "act -j coverage-linux -W .github\workflows\coverage.yml --pull=false"

# 3. Lint (ShellCheck)
Run-Test "Lint (ShellCheck)" `
    "act -j shellcheck -W .github\workflows\lint.yaml --pull=false"

# Summary
Write-Host "==========================================" -ForegroundColor Blue
Write-Host "Test Results Summary:"
Write-Host "  ✓ Passed:   $script:PASSED" -ForegroundColor Green
Write-Host "  ✗ Failed:   $script:FAILED" -ForegroundColor Red
Write-Host "==========================================" -ForegroundColor Blue

if ($script:FAILED -eq 0) {
    Write-Success "All tests passed! Ready to push to GitHub."
    exit 0
} else {
    Write-Error-Msg "$script:FAILED test(s) failed. Please fix before pushing."
    exit 1
}
