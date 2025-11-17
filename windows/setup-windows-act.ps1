#Requires -RunAsAdministrator

<#
.SYNOPSIS
    Automated setup script for Windows testing with act

.DESCRIPTION
    This script automates the setup of a Windows 11 machine for GitHub Actions
    testing using act, controlled remotely from Mac via SSH.

.PARAMETER MacPublicKey
    SSH public key from the Mac controller (required)

.PARAMETER SkipSoftwareInstall
    Skip software installation (if already installed)

.PARAMETER WorkspacePath
    Path for act workspace (default: C:\act-workspace)

.EXAMPLE
    .\setup-windows-act.ps1 -MacPublicKey "ssh-ed25519 AAAAC3Nza... mac-to-windows-act"

.EXAMPLE
    .\setup-windows-act.ps1 -MacPublicKey "ssh-ed25519 AAAAC3Nza..." -SkipSoftwareInstall

.NOTES
    Author: OBS Polyemesis Team
    Requires: Windows 11, Administrator privileges
    Version: 1.0
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$MacPublicKey,

    [Parameter(Mandatory=$false)]
    [switch]$SkipSoftwareInstall,

    [Parameter(Mandatory=$false)]
    [string]$WorkspacePath = "C:\act-workspace"
)

# Color output functions
function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

function Write-Info {
    param([string]$Message)
    Write-Host "ℹ $Message" -ForegroundColor Cyan
}

function Write-Warning {
    param([string]$Message)
    Write-Host "⚠ $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor Red
}

function Write-Step {
    param([string]$Message)
    Write-Host "`n=== $Message ===" -ForegroundColor Magenta
}

# Check if running as Administrator
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Error "This script must be run as Administrator"
    exit 1
}

Write-Info "Starting Windows testing environment setup..."
Write-Info "Workspace path: $WorkspacePath"

# Step 1: Install Software
if (-not $SkipSoftwareInstall) {
    Write-Step "Installing Required Software"

    # Check if winget is available
    try {
        $null = winget --version
        Write-Success "winget is available"
    } catch {
        Write-Error "winget is not available. Please update Windows or install App Installer from Microsoft Store"
        exit 1
    }

    # Install Git
    Write-Info "Installing Git for Windows..."
    try {
        winget install --id Git.Git --silent --accept-package-agreements --accept-source-agreements
        Write-Success "Git installed"
    } catch {
        Write-Warning "Git installation failed or already installed: $_"
    }

    # Install Visual Studio 2022 Community
    Write-Info "Installing Visual Studio 2022 Community (this may take a while)..."
    try {
        winget install --id Microsoft.VisualStudio.2022.Community --silent --accept-package-agreements --accept-source-agreements
        Write-Success "Visual Studio 2022 installed"
        Write-Warning "You may need to manually add C++ Desktop Development workload"
    } catch {
        Write-Warning "Visual Studio installation failed or already installed: $_"
    }

    # Install CMake
    Write-Info "Installing CMake..."
    try {
        winget install --id Kitware.CMake --silent --accept-package-agreements --accept-source-agreements
        Write-Success "CMake installed"
    } catch {
        Write-Warning "CMake installation failed or already installed: $_"
    }

    # Install NSIS
    Write-Info "Installing NSIS..."
    try {
        winget install --id NSIS.NSIS --silent --accept-package-agreements --accept-source-agreements
        Write-Success "NSIS installed"
    } catch {
        Write-Warning "NSIS installation failed or already installed: $_"
    }

    # Install Docker Desktop
    Write-Info "Installing Docker Desktop..."
    try {
        winget install --id Docker.DockerDesktop --silent --accept-package-agreements --accept-source-agreements
        Write-Success "Docker Desktop installed"
        Write-Warning "Docker Desktop requires a restart. Please restart Windows and run Docker Desktop manually."
    } catch {
        Write-Warning "Docker installation failed or already installed: $_"
    }

    # Install act
    Write-Info "Installing act..."
    try {
        winget install --id nektos.act --silent --accept-package-agreements --accept-source-agreements
        Write-Success "act installed"
    } catch {
        Write-Warning "act installation failed or already installed: $_"
    }

    Write-Success "Software installation completed"
} else {
    Write-Info "Skipping software installation (--SkipSoftwareInstall specified)"
}

# Step 2: Configure SSH Server
Write-Step "Configuring SSH Server"

# Check if OpenSSH Server is installed
$sshServerFeature = Get-WindowsCapability -Online | Where-Object Name -like 'OpenSSH.Server*'

if ($sshServerFeature.State -ne "Installed") {
    Write-Info "Installing OpenSSH Server..."
    Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0
    Write-Success "OpenSSH Server installed"
} else {
    Write-Success "OpenSSH Server already installed"
}

# Start and configure SSH service
Write-Info "Starting and configuring SSH service..."
Start-Service sshd
Set-Service -Name sshd -StartupType 'Automatic'
Write-Success "SSH service configured to start automatically"

# Configure firewall for SSH
Write-Info "Configuring firewall for SSH..."
$firewallRule = Get-NetFirewallRule -Name *ssh* -ErrorAction SilentlyContinue

if (-not $firewallRule) {
    New-NetFirewallRule -Name sshd -DisplayName 'OpenSSH Server (sshd)' -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 22
    Write-Success "SSH firewall rule created"
} else {
    Write-Success "SSH firewall rule already exists"
}

# Step 3: Configure SSH Keys
Write-Step "Configuring SSH Authentication"

# Create .ssh directory if it doesn't exist
$sshDir = "$env:USERPROFILE\.ssh"
if (-not (Test-Path $sshDir)) {
    New-Item -ItemType Directory -Force -Path $sshDir | Out-Null
    Write-Success "Created .ssh directory"
}

# Set proper permissions on .ssh directory
icacls $sshDir /inheritance:r | Out-Null
icacls $sshDir /grant:r "$env:USERNAME:(OI)(CI)F" | Out-Null
Write-Success ".ssh directory permissions configured"

# Add Mac public key to authorized_keys
$authorizedKeysPath = "$sshDir\authorized_keys"

if (Test-Path $authorizedKeysPath) {
    Write-Warning "authorized_keys file already exists"
    $existingKeys = Get-Content $authorizedKeysPath
    if ($existingKeys -contains $MacPublicKey) {
        Write-Info "Mac public key already in authorized_keys"
    } else {
        Add-Content -Path $authorizedKeysPath -Value $MacPublicKey
        Write-Success "Mac public key added to authorized_keys"
    }
} else {
    Set-Content -Path $authorizedKeysPath -Value $MacPublicKey
    Write-Success "Created authorized_keys with Mac public key"
}

# Set permissions on authorized_keys
icacls $authorizedKeysPath /inheritance:r | Out-Null
icacls $authorizedKeysPath /grant:r "$env:USERNAME:F" | Out-Null
Write-Success "authorized_keys permissions configured"

# Step 4: Create Workspace Directory
Write-Step "Creating Workspace Directory"

if (-not (Test-Path $WorkspacePath)) {
    New-Item -ItemType Directory -Force -Path $WorkspacePath | Out-Null
    Write-Success "Created workspace directory: $WorkspacePath"
} else {
    Write-Success "Workspace directory already exists: $WorkspacePath"
}

# Create obs-polyemesis subdirectory
$projectPath = "$WorkspacePath\obs-polyemesis"
if (-not (Test-Path $projectPath)) {
    New-Item -ItemType Directory -Force -Path $projectPath | Out-Null
    Write-Success "Created project directory: $projectPath"
}

# Create cache directory
$cachePath = "C:\act-cache"
if (-not (Test-Path $cachePath)) {
    New-Item -ItemType Directory -Force -Path $cachePath | Out-Null
    Write-Success "Created cache directory: $cachePath"
}

# Step 5: Configure act
Write-Step "Configuring act"

# Copy .actrc to user profile
$actrcSource = "$projectPath\windows\.actrc"
$actrcDest = "$env:USERPROFILE\.actrc"

if (Test-Path $actrcSource) {
    Copy-Item -Path $actrcSource -Destination $actrcDest -Force
    Write-Success "Copied .actrc to user profile"
} else {
    Write-Warning ".actrc not found in project. You'll need to copy it manually later."
}

# Step 6: Configure Git
Write-Step "Configuring Git"

$gitConfigured = $false
try {
    $gitUser = git config --global user.name
    if ($gitUser) {
        Write-Success "Git already configured with user: $gitUser"
        $gitConfigured = $true
    }
} catch {
    # Git not configured
}

if (-not $gitConfigured) {
    Write-Warning "Git is not configured. Please run:"
    Write-Host '  git config --global user.name "Your Name"' -ForegroundColor Yellow
    Write-Host '  git config --global user.email "your.email@example.com"' -ForegroundColor Yellow
}

# Step 7: Verify Docker
Write-Step "Verifying Docker Installation"

try {
    $dockerVersion = docker --version
    Write-Success "Docker is installed: $dockerVersion"

    # Check if Docker is running
    $dockerInfo = docker info 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Success "Docker is running"
    } else {
        Write-Warning "Docker is installed but not running. Please start Docker Desktop."
    }
} catch {
    Write-Warning "Docker is not available. Please install Docker Desktop and restart Windows."
}

# Step 8: Display Network Information
Write-Step "Network Information"

$ipAddress = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.InterfaceAlias -notlike "*Loopback*" -and $_.IPAddress -notlike "169.254.*" } | Select-Object -First 1).IPAddress

Write-Info "Windows IP Address: $ipAddress"
Write-Info "Use this IP address to configure SSH on your Mac"

# Step 9: Summary and Next Steps
Write-Step "Setup Complete!"

Write-Host "`nSetup Summary:" -ForegroundColor Green
Write-Host "  ✓ SSH Server configured and running"
Write-Host "  ✓ Mac public key added to authorized_keys"
Write-Host "  ✓ Workspace directory created: $WorkspacePath"
Write-Host "  ✓ Cache directory created: $cachePath"
Write-Host "  ✓ Firewall configured for SSH (port 22)"

Write-Host "`nNext Steps:" -ForegroundColor Cyan
Write-Host "  1. Restart Windows (if Docker was installed)"
Write-Host "  2. Start Docker Desktop manually"
Write-Host "  3. Test SSH connection from Mac:"
Write-Host "       ssh $env:USERNAME@$ipAddress" -ForegroundColor Yellow
Write-Host "  4. Configure Mac SSH client (~/.ssh/config):"
Write-Host "       Host windows-act" -ForegroundColor Yellow
Write-Host "           HostName $ipAddress" -ForegroundColor Yellow
Write-Host "           User $env:USERNAME" -ForegroundColor Yellow
Write-Host "           IdentityFile ~/.ssh/id_ed25519_windows" -ForegroundColor Yellow
Write-Host "  5. Run helper scripts from Mac to test workflows"

Write-Host "`nFor manual testing on Windows:" -ForegroundColor Cyan
Write-Host "  cd $projectPath"
Write-Host "  act -l  # List workflows"
Write-Host "  act -W .github/workflows/create-packages.yaml  # Run workflow"

Write-Host "`nTroubleshooting:" -ForegroundColor Cyan
Write-Host "  - View SSH logs: Get-EventLog -LogName Application -Source sshd -Newest 20"
Write-Host "  - Check SSH service: Get-Service sshd"
Write-Host "  - Test Docker: docker run hello-world"
Write-Host "  - Check firewall: Get-NetFirewallRule -Name *ssh*"

Write-Success "`nSetup completed successfully!"
