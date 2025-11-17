# OBS Polyemesis - Windows End-to-End Test Suite
# Complete plugin lifecycle testing: build -> install -> load -> test -> cleanup

param(
    [string]$TestMode = "full",  # full, quick, install-only, ci
    [switch]$RemovePlugin,
    [switch]$Help
)

# Configuration
$PLUGIN_NAME = "obs-polyemesis.dll"
$OBS_PLUGIN_PATH = if ($env:OBS_PLUGIN_PATH) { $env:OBS_PLUGIN_PATH } else { "$env:APPDATA\obs-studio\plugins" }
$OBS_LOGS_PATH = "$env:APPDATA\obs-studio\logs"
$PROJECT_ROOT = (Get-Item $PSScriptRoot).Parent.Parent.Parent.FullName
$BUILD_DIR = Join-Path $PROJECT_ROOT "build"
$E2E_WORKSPACE = if ($env:E2E_WORKSPACE) { $env:E2E_WORKSPACE } else { "$env:TEMP\obs-polyemesis-e2e" }

# Test state
$script:TestsRun = 0
$script:TestsPassed = 0
$script:TestsFailed = 0
$script:StartTime = Get-Date

# Colors
function Write-Info { param($msg) Write-Host "[INFO] $msg" -ForegroundColor Blue }
function Write-Success { param($msg) Write-Host "[SUCCESS] $msg" -ForegroundColor Green }
function Write-Error { param($msg) Write-Host "[ERROR] $msg" -ForegroundColor Red }
function Write-Warning { param($msg) Write-Host "[WARNING] $msg" -ForegroundColor Yellow }
function Write-Stage {
    param($name, $num)
    Write-Host ""
    Write-Host "[STAGE $num] $name" -ForegroundColor Cyan
    Write-Host "----------------------------------------" -ForegroundColor Cyan
}

# Show help
if ($Help) {
    Write-Host "Usage: .\e2e-test-windows.ps1 [OPTIONS]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -TestMode MODE    Test mode: full, quick, install-only, ci (default: full)"
    Write-Host "  -RemovePlugin     Remove plugin after tests"
    Write-Host "  -Help             Show this help message"
    exit 0
}

# Test assertions
function Assert-FileExists {
    param([string]$Path, [string]$Description)

    $script:TestsRun++

    if (Test-Path -Path $Path -PathType Leaf) {
        Write-Success "✓ $Description"
        $script:TestsPassed++
        return $true
    } else {
        Write-Error "✗ $Description"
        Write-Error "  File not found: $Path"
        $script:TestsFailed++
        return $false
    }
}

function Assert-DirExists {
    param([string]$Path, [string]$Description)

    $script:TestsRun++

    if (Test-Path -Path $Path -PathType Container) {
        Write-Success "✓ $Description"
        $script:TestsPassed++
        return $true
    } else {
        Write-Error "✗ $Description"
        Write-Error "  Directory not found: $Path"
        $script:TestsFailed++
        return $false
    }
}

function Assert-CommandSuccess {
    param([scriptblock]$Command, [string]$Description)

    $script:TestsRun++

    try {
        & $Command | Out-Null
        if ($LASTEXITCODE -eq 0 -or $null -eq $LASTEXITCODE) {
            Write-Success "✓ $Description"
            $script:TestsPassed++
            return $true
        } else {
            Write-Error "✗ $Description"
            $script:TestsFailed++
            return $false
        }
    } catch {
        Write-Error "✗ $Description"
        Write-Error "  Error: $_"
        $script:TestsFailed++
        return $false
    }
}

# Print header
function Print-Header {
    Write-Host "========================================"
    Write-Host " OBS Polyemesis - End-to-End Tests"
    Write-Host "========================================"
    Write-Host ""
    Write-Host "Platform: Windows ($env:PROCESSOR_ARCHITECTURE)"
    Write-Host "Test Mode: $TestMode"
    Write-Host "Date: $(Get-Date)"
    Write-Host ""
}

# Stage 1: Build Verification
function Stage-BuildVerification {
    Write-Stage "Build Verification" "1/7"

    if ($TestMode -eq "quick") {
        Write-Info "Skipping build in quick mode"
        Assert-FileExists "$BUILD_DIR\Release\$PLUGIN_NAME" "Build output exists"
        return
    }

    # Clean previous build
    if (Test-Path $BUILD_DIR) {
        Write-Info "Cleaning previous build..."
        Remove-Item -Recurse -Force $BUILD_DIR
    }

    # Build plugin
    Write-Info "Building plugin..."
    Push-Location $PROJECT_ROOT

    # CMake configuration
    $configLog = Join-Path $E2E_WORKSPACE "cmake-config.log"
    cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
        -DCMAKE_BUILD_TYPE=Release `
        -DENABLE_SCRIPTING=OFF > $configLog 2>&1

    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed"
        Get-Content $configLog
        Pop-Location
        return
    }

    # Build
    $buildLog = Join-Path $E2E_WORKSPACE "cmake-build.log"
    cmake --build build --config Release > $buildLog 2>&1

    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed"
        Get-Content $buildLog
        Pop-Location
        return
    }

    Pop-Location

    # Verify build output
    Assert-FileExists "$BUILD_DIR\Release\$PLUGIN_NAME" "Plugin binary created"

    # Check binary type
    $binary = Join-Path $BUILD_DIR "Release\$PLUGIN_NAME"
    $fileInfo = (Get-Item $binary).VersionInfo

    if ($fileInfo) {
        Write-Success "✓ Binary is valid PE file"
        $script:TestsRun++
        $script:TestsPassed++
    }

    # Check architecture
    $peInfo = dumpbin /headers $binary 2>&1 | Select-String "machine"
    if ($peInfo -match "x64") {
        Write-Success "✓ Binary is x64"
        $script:TestsRun++
        $script:TestsPassed++
    }

    # Check dependencies
    $deps = dumpbin /dependents $binary 2>&1

    if ($deps -match "obs\.dll|libobs") {
        Write-Success "✓ OBS library dependency found"
        $script:TestsRun++
        $script:TestsPassed++
    }

    if ($deps -match "libcurl") {
        Write-Success "✓ libcurl dependency found"
        $script:TestsRun++
        $script:TestsPassed++
    }
}

# Stage 2: Installation
function Stage-Installation {
    Write-Stage "Installation" "2/7"

    # Create plugin directory
    Assert-CommandSuccess { New-Item -ItemType Directory -Force -Path $OBS_PLUGIN_PATH | Out-Null } "Plugin directory created"

    # Remove old installation
    $installedPlugin = Join-Path $OBS_PLUGIN_PATH $PLUGIN_NAME
    if (Test-Path $installedPlugin) {
        Write-Info "Removing previous installation..."
        Remove-Item -Force $installedPlugin
    }

    # Install plugin
    Write-Info "Installing plugin to OBS..."
    $sourcePlugin = Join-Path $BUILD_DIR "Release\$PLUGIN_NAME"
    Assert-CommandSuccess { Copy-Item $sourcePlugin $OBS_PLUGIN_PATH } "Plugin file copied"

    # Verify installation
    Assert-FileExists $installedPlugin "Plugin installed"

    # Check file size
    $fileSize = (Get-Item $installedPlugin).Length
    if ($fileSize -gt 0) {
        Write-Success "✓ Plugin file size: $([math]::Round($fileSize / 1KB, 2)) KB"
        $script:TestsRun++
        $script:TestsPassed++
    }
}

# Stage 3: Plugin Loading Verification
function Stage-PluginLoading {
    Write-Stage "Plugin Loading Verification" "3/7"

    if ($TestMode -eq "install-only") {
        Write-Info "Skipping plugin loading in install-only mode"
        return
    }

    # Create logs directory if it doesn't exist
    New-Item -ItemType Directory -Force -Path $OBS_LOGS_PATH | Out-Null

    # Check for recent OBS logs
    $latestLog = Get-ChildItem $OBS_LOGS_PATH -Filter "*.txt" | Sort-Object LastWriteTime -Descending | Select-Object -First 1

    if ($latestLog) {
        Write-Info "Checking OBS logs: $($latestLog.Name)"

        $logContent = Get-Content $latestLog.FullName -Raw

        # Check if plugin is mentioned in logs
        if ($logContent -match "obs-polyemesis|polyemesis") {
            Write-Success "✓ Plugin mentioned in OBS logs"
            $script:TestsRun++
            $script:TestsPassed++

            # Check for load errors
            $errors = Select-String -Path $latestLog.FullName -Pattern "obs-polyemesis.*error" -CaseSensitive:$false
            if ($errors -and ($errors | Where-Object { $_ -notmatch "0=INACTIVE.*ERROR" })) {
                Write-Warning "⚠ Potential errors found in logs"
                $errors | Select-Object -First 5 | ForEach-Object { Write-Warning "  $_" }
            } else {
                Write-Success "✓ No plugin errors in logs"
                $script:TestsRun++
                $script:TestsPassed++
            }

            # Check for module load
            if ($logContent -match "obs_module_load.*polyemesis|Loaded module.*polyemesis") {
                Write-Success "✓ Plugin module loaded"
                $script:TestsRun++
                $script:TestsPassed++
            }
        } else {
            Write-Warning "⚠ Plugin not found in logs (OBS may not have been run yet)"
            Write-Info "  Run OBS Studio manually to generate logs, then re-run tests"
        }
    } else {
        Write-Warning "⚠ No OBS logs found"
        Write-Info "  Run OBS Studio at least once to generate logs"
    }
}

# Stage 4: Binary Symbols Verification
function Stage-BinarySymbols {
    Write-Stage "Binary Symbols Verification" "4/7"

    $binary = Join-Path $OBS_PLUGIN_PATH $PLUGIN_NAME

    # Check for required OBS symbols using dumpbin
    $exports = dumpbin /exports $binary 2>&1

    if ($exports -match "obs_module_load") {
        Write-Success "✓ obs_module_load symbol exported"
        $script:TestsRun++
        $script:TestsPassed++
    } else {
        Write-Error "✗ obs_module_load symbol missing"
        $script:TestsRun++
        $script:TestsFailed++
    }

    if ($exports -match "obs_module_unload") {
        Write-Success "✓ obs_module_unload symbol exported"
        $script:TestsRun++
        $script:TestsPassed++
    }

    # Check for jansson symbols
    if ($exports -match "json_") {
        Write-Success "✓ JSON symbols present (jansson linked)"
        $script:TestsRun++
        $script:TestsPassed++
    }

    # Check for curl symbols
    if ($exports -match "curl_") {
        Write-Success "✓ curl symbols present"
        $script:TestsRun++
        $script:TestsPassed++
    }
}

# Stage 5: Dependencies Verification
function Stage-Dependencies {
    Write-Stage "Dependencies Verification" "5/7"

    $binary = Join-Path $OBS_PLUGIN_PATH $PLUGIN_NAME

    # Check dependencies using dumpbin
    $deps = dumpbin /dependents $binary 2>&1

    # Extract DLL dependencies
    $dllList = $deps | Select-String "\.dll" | ForEach-Object { $_.Line.Trim() }

    if ($dllList) {
        Write-Info "Plugin dependencies:"
        $dllList | ForEach-Object { Write-Info "  - $_" }
    }

    # Check for required dependencies
    $requiredDeps = @("obs.dll", "KERNEL32.dll", "VCRUNTIME140.dll")
    $foundCount = 0

    foreach ($dep in $requiredDeps) {
        if ($deps -match [regex]::Escape($dep)) {
            $foundCount++
        }
    }

    if ($foundCount -ge 2) {
        Write-Success "✓ Core dependencies found ($foundCount/$($requiredDeps.Count))"
        $script:TestsRun++
        $script:TestsPassed++
    }

    # Check for Qt dependencies
    if ($deps -match "Qt\d+") {
        Write-Success "✓ Qt libraries linked"
        $script:TestsRun++
        $script:TestsPassed++
    }
}

# Stage 6: File Properties Verification
function Stage-FileProperties {
    Write-Stage "File Properties Verification" "6/7"

    $binary = Join-Path $OBS_PLUGIN_PATH $PLUGIN_NAME

    # Check file version info
    $fileInfo = (Get-Item $binary).VersionInfo

    if ($fileInfo.FileVersion) {
        Write-Success "✓ File version: $($fileInfo.FileVersion)"
        $script:TestsRun++
        $script:TestsPassed++
    }

    if ($fileInfo.ProductName) {
        Write-Success "✓ Product name: $($fileInfo.ProductName)"
        $script:TestsRun++
        $script:TestsPassed++
    }

    if ($fileInfo.LegalCopyright) {
        Write-Success "✓ Copyright: $($fileInfo.LegalCopyright)"
        $script:TestsRun++
        $script:TestsPassed++
    }

    # Check digital signature (if present)
    $signature = Get-AuthenticodeSignature $binary
    if ($signature.Status -eq "Valid") {
        Write-Success "✓ Binary is digitally signed"
        $script:TestsRun++
        $script:TestsPassed++
    } elseif ($signature.Status -eq "NotSigned") {
        Write-Warning "⚠ Binary not digitally signed (expected for local builds)"
    }
}

# Stage 7: Cleanup
function Stage-Cleanup {
    Write-Stage "Cleanup" "7/7"

    # Kill any running OBS instances (if in CI mode)
    if ($TestMode -eq "ci") {
        $obsProcess = Get-Process obs -ErrorAction SilentlyContinue
        if ($obsProcess) {
            Write-Info "Stopping OBS process..."
            Stop-Process -Name obs -Force -ErrorAction SilentlyContinue
            Start-Sleep -Seconds 2
        }

        if (-not (Get-Process obs -ErrorAction SilentlyContinue)) {
            Write-Success "✓ OBS process stopped"
            $script:TestsRun++
            $script:TestsPassed++
        }
    }

    # Optional: Remove plugin installation
    if ($TestMode -eq "ci" -or $RemovePlugin) {
        Write-Info "Removing plugin installation..."
        $installedPlugin = Join-Path $OBS_PLUGIN_PATH $PLUGIN_NAME
        if (Test-Path $installedPlugin) {
            Remove-Item -Force $installedPlugin
            Write-Success "✓ Plugin uninstalled"
            $script:TestsRun++
            $script:TestsPassed++
        }
    } else {
        Write-Info "Keeping plugin installed for manual testing"
        Write-Info "  Plugin location: $(Join-Path $OBS_PLUGIN_PATH $PLUGIN_NAME)"
        Write-Info "  To uninstall: Remove-Item `"$(Join-Path $OBS_PLUGIN_PATH $PLUGIN_NAME)`""
    }

    # Cleanup workspace
    if (Test-Path $E2E_WORKSPACE) {
        if ($script:TestsFailed -eq 0 -or $TestMode -eq "ci") {
            Write-Info "Cleaning up workspace: $E2E_WORKSPACE"
            Remove-Item -Recurse -Force $E2E_WORKSPACE -ErrorAction SilentlyContinue
        } else {
            Write-Warning "Keeping workspace due to test failures: $E2E_WORKSPACE"
        }
    }
}

# Print test summary
function Print-TestSummary {
    $duration = (Get-Date) - $script:StartTime

    Write-Host ""
    Write-Host "========================================"
    Write-Host " Test Summary"
    Write-Host "========================================"
    Write-Host ""
    Write-Host "Total Tests: $($script:TestsRun)"
    Write-Host "Passed:      $($script:TestsPassed)"
    Write-Host "Failed:      $($script:TestsFailed)"
    Write-Host "Duration:    $([math]::Round($duration.TotalSeconds, 1))s"
    Write-Host ""

    if ($script:TestsFailed -eq 0) {
        Write-Success "✅ All end-to-end tests passed!"
        return 0
    } else {
        Write-Error "❌ Some end-to-end tests failed"
        return 1
    }
}

# Main execution
function Main {
    Print-Header

    # Create workspace
    New-Item -ItemType Directory -Force -Path $E2E_WORKSPACE | Out-Null
    Write-Info "Test workspace: $E2E_WORKSPACE"

    # Run test stages
    try {
        Stage-BuildVerification
        Stage-Installation
        Stage-PluginLoading
        Stage-BinarySymbols
        Stage-Dependencies
        Stage-FileProperties
        Stage-Cleanup
    } catch {
        Write-Error "Test execution failed: $_"
        $script:TestsFailed++
    }

    # Print summary
    $exitCode = Print-TestSummary

    # Instructions for next steps
    if ($exitCode -eq 0) {
        Write-Host ""
        Write-Host "Next steps:"
        Write-Host "1. Start OBS Studio to verify plugin loads correctly"
        Write-Host "2. Check View menu for 'Restreamer Control' dock"
        Write-Host "3. Add a Restreamer source to verify functionality"
        Write-Host ""
    }

    exit $exitCode
}

# Run main
Main
