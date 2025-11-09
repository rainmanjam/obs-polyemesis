# Windows Installer - OBS Polyemesis

This directory contains the NSIS script for creating a Windows installer for the OBS Polyemesis plugin.

## Prerequisites

1. **NSIS (Nullsoft Scriptable Install System)**
   - Download from: https://nsis.sourceforge.io/Download
   - Install NSIS 3.x or later
   - Add NSIS to your PATH (usually `C:\Program Files (x86)\NSIS`)

2. **Built Plugin**
   - The plugin must be built first in Release configuration
   - Build location: `.build\windows\Release\obs-polyemesis\`

## Building the Installer

### Option 1: Using Command Line

```powershell
# From the project root directory
cd packaging\windows

# Build with default version (1.0.0)
makensis installer.nsi

# Build with specific version
makensis /DVERSION=1.0.0 installer.nsi
```

### Option 2: Using NSIS GUI

1. Open NSIS
2. Load `packaging\windows\installer.nsi`
3. Click "Compile NSI scripts"

## Output

The installer will be created at:
```
build\installers\obs-polyemesis-{VERSION}-windows-x64.exe
```

## Installer Features

- **OBS Detection**: Checks if OBS Studio is installed
- **Upgrade Detection**: Detects existing installations
- **Registry Integration**: Adds Windows uninstaller entry
- **Components**:
  - Core plugin files (required)
  - Locale files (optional)
- **Installation Path**: `%APPDATA%\obs-studio\plugins\obs-polyemesis`

## Installation Process

1. Double-click the `.exe` installer
2. Follow the installation wizard
3. Choose components to install
4. Click Install
5. Restart OBS Studio if running

## Uninstallation

Users can uninstall via:
- Windows Settings → Apps → OBS Polyemesis
- Or run `uninst.exe` from the installation directory

## Testing

After building the installer:

1. Run the installer on a clean Windows machine
2. Verify files are installed to `%APPDATA%\obs-studio\plugins\obs-polyemesis`
3. Launch OBS Studio
4. Check that the plugin appears in View → Docks → Restreamer Control
5. Test uninstallation

## Troubleshooting

**Error: Can't open script file**
- Ensure you're running from the `packaging\windows` directory
- Or use absolute paths

**Error: Output file already exists**
- Delete the existing installer from `build\installers\`

**Plugin not appearing in OBS**
- Verify installation directory: `%APPDATA%\obs-studio\plugins\`
- Check OBS Studio is version 28.0 or later
- Restart OBS Studio after installation

## Customization

To customize the installer, edit `installer.nsi` and modify:

- `PRODUCT_NAME`: Plugin name
- `PRODUCT_PUBLISHER`: Your name/organization
- `PRODUCT_WEB_SITE`: Project URL
- `InstallDir`: Default installation directory
- Pages: Welcome, license, components, etc.

## Requirements

- Windows 7 or later (installer runtime)
- NSIS 3.x (build time)
- Built plugin binaries
- LICENSE file in project root

## Notes

- The installer is built for x64 architecture
- Requires user-level permissions (no admin required)
- Install location is per-user (`%APPDATA%`)
- Supports silent installation: `/S` flag
