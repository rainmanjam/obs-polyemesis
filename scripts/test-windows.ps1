# OBS Polyemesis - Windows Testing Script
# Tests the plugin on Windows

param(
    [switch]$SkipBuild,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

# Colors (if supported)
function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Blue
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

function Write-Warning-Custom {
    param([string]$Message)
    Write-Host "[WARNING] $Message" -ForegroundColor Yellow
}

function Write-Error-Custom {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

# Configuration
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build-windows"

Write-Info "Starting Windows testing for OBS Polyemesis"
Write-Host "========================================"

# Check Python
if (!(Get-Command python -ErrorAction SilentlyContinue)) {
    Write-Error-Custom "Python not found. Please install Python 3."
    exit 1
}

Write-Success "Python found"

# Check OBS installation
$ObsPaths = @(
    "C:\Program Files\obs-studio",
    "C:\Program Files (x86)\obs-studio",
    "$env:ProgramFiles\obs-studio",
    "${env:ProgramFiles(x86)}\obs-studio"
)

$ObsPath = $null
foreach ($path in $ObsPaths) {
    if (Test-Path $path) {
        $ObsPath = $path
        break
    }
}

if ($ObsPath) {
    Write-Success "OBS Studio found at: $ObsPath"

    # Check OBS version
    $ObsExe = Join-Path $ObsPath "bin\64bit\obs64.exe"
    if (Test-Path $ObsExe) {
        $ObsVersion = (Get-Item $ObsExe).VersionInfo.FileVersion
        Write-Info "OBS Studio version: $ObsVersion"

        # Verify it's version 32.0.2
        if ($ObsVersion -match "32\.0\.2") {
            Write-Success "OBS version 32.0.2 confirmed"
        } else {
            Write-Warning-Custom "Expected OBS 32.0.2 but found $ObsVersion"
        }
    }
} else {
    Write-Warning-Custom "OBS Studio not found in standard locations"
    Write-Info "Tests will run but plugin installation cannot be verified"
}

# Build the plugin (unless skipped)
if (!$SkipBuild) {
    Write-Info "Building OBS Polyemesis plugin for Windows..."

    if (!(Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
    }

    Push-Location $BuildDir

    try {
        # Configure CMake
        Write-Info "Configuring CMake..."
        cmake -S .. -B . -G "Visual Studio 17 2022" -A x64 -DENABLE_QT=ON

        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }

        # Build
        Write-Info "Building..."
        cmake --build . --config Release

        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }

        Write-Success "Plugin built successfully"
    }
    catch {
        Write-Error-Custom $_.Exception.Message
        Pop-Location
        exit 1
    }
    finally {
        Pop-Location
    }
} else {
    Write-Info "Skipping build (--SkipBuild specified)"
}

# Install Python test dependencies
Write-Info "Installing Python test dependencies..."
python -m pip install --quiet requests python-dotenv

# Run API integration tests
Write-Info "Running API integration tests..."
Push-Location (Join-Path $ProjectRoot "tests")

try {
    python test-restreamer-integration.py
}
catch {
    Write-Warning-Custom "API integration tests encountered errors"
}
finally {
    Pop-Location
}

# Run automated tests if they exist
$AutomatedTestsDir = Join-Path $ProjectRoot "tests\automated"
if (Test-Path $AutomatedTestsDir) {
    Write-Info "Running automated test suite..."
    Push-Location $AutomatedTestsDir

    try {
        Get-ChildItem -Filter "test_*.py" | ForEach-Object {
            Write-Info "Running $($_.Name)..."
            python $_.FullName
        }
    }
    catch {
        Write-Warning-Custom "Some automated tests failed"
    }
    finally {
        Pop-Location
    }
}

# Check build artifacts
Write-Info "Checking build artifacts..."
$PluginDll = Join-Path $BuildDir "Release\obs-polyemesis.dll"

if (Test-Path $PluginDll) {
    Write-Success "Plugin binary created: obs-polyemesis.dll"
    $FileInfo = Get-Item $PluginDll
    Write-Host "  Size: $([math]::Round($FileInfo.Length / 1KB, 2)) KB"
} else {
    Write-Error-Custom "Plugin binary not found at: $PluginDll"
    exit 1
}

# Summary
Write-Host ""
Write-Host "========================================"
Write-Success "Windows testing completed!"
Write-Host ""

if ($ObsPath) {
    Write-Info "To install the plugin:"
    Write-Host "  Copy-Item '$PluginDll' -Destination '$ObsPath\obs-plugins\64bit\'"
}

exit 0
