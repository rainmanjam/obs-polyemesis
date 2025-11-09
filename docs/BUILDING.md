# Building OBS Polyemesis

This guide provides instructions for building the OBS Polyemesis plugin on all supported platforms.

## Table of Contents

- [Prerequisites](#prerequisites)
- [macOS Build Instructions](#macos-build-instructions)
- [Linux Build Instructions](#linux-build-instructions)
- [Windows Build Instructions](#windows-build-instructions)
- [Cross-Platform CI/CD](#cross-platform-cicd)
- [Testing Locally with ACT](#testing-locally-with-act)

---

## Prerequisites

All platforms require:
- **CMake 3.28 or newer**
- **Git**
- **C/C++ compiler** (Clang, GCC, or MSVC)
- **OBS Studio** (for testing)

Platform-specific dependencies are detailed below.

---

## macOS Build Instructions

### Dependencies

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required dependencies
brew install cmake qt jansson curl
```

### Building

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/obs-polyemesis.git
cd obs-polyemesis

# Configure the build (Universal Binary: arm64 + x86_64)
cmake -B build -G Xcode \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_FRONTEND_API=ON \
  -DENABLE_QT=ON

# Build the plugin
cmake --build build --config RelWithDebInfo

# The plugin will be located at:
# build/RelWithDebInfo/obs-polyemesis.plugin/
```

### Installation

```bash
# Copy to OBS plugins directory
cp -r build/RelWithDebInfo/obs-polyemesis.plugin \
  ~/Library/Application\ Support/obs-studio/plugins/
```

### Notes

- **Qt6 from Homebrew** is used to avoid deprecated AGL framework issues in OBS's pre-built dependencies
- Builds universal binaries (arm64 + x86_64) by default
- Requires macOS 11.0 (Big Sur) or later

---

## Linux Build Instructions

### Ubuntu/Debian

#### Dependencies

```bash
# Update package list
sudo apt update

# Install build dependencies
sudo apt install -y \
  build-essential \
  cmake \
  git \
  qtbase6-dev \
  qt6-base-dev \
  libjansson-dev \
  libcurl4-openssl-dev \
  obs-studio \
  libobs-dev

# If libobs-dev is not available, you may need to build OBS from source
```

#### Building

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/obs-polyemesis.git
cd obs-polyemesis

# Configure the build
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_FRONTEND_API=ON \
  -DENABLE_QT=ON

# Build the plugin
cmake --build build --config Release -j$(nproc)

# The plugin will be located at:
# build/obs-polyemesis.so
```

#### Installation

```bash
# Install to user plugins directory
mkdir -p ~/.config/obs-studio/plugins/obs-polyemesis/bin/64bit/
cp build/obs-polyemesis.so ~/.config/obs-studio/plugins/obs-polyemesis/bin/64bit/

# Copy data files
mkdir -p ~/.config/obs-studio/plugins/obs-polyemesis/data/
cp -r data/* ~/.config/obs-studio/plugins/obs-polyemesis/data/
```

### Fedora/RHEL

```bash
# Install dependencies
sudo dnf install -y \
  cmake \
  gcc \
  gcc-c++ \
  qt6-qtbase-devel \
  jansson-devel \
  libcurl-devel \
  obs-studio-devel

# Follow the same build steps as Ubuntu
```

### Arch Linux

```bash
# Install dependencies
sudo pacman -S cmake qt6-base jansson curl obs-studio

# Follow the same build steps as Ubuntu
```

### ARM Linux (Raspberry Pi, etc.)

The build process is the same as above. The CMake configuration will automatically detect ARM architecture and build accordingly.

```bash
# For 64-bit ARM (aarch64)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# The plugin will be built for the native architecture
```

---

## Windows Build Instructions

### Dependencies

1. **Install Visual Studio 2022** (Community Edition is free)
   - Include "Desktop development with C++" workload
   - Download from: https://visualstudio.microsoft.com/

2. **Install CMake**
   - Download from: https://cmake.org/download/
   - Or use: `winget install Kitware.CMake`

3. **Install Git**
   - Download from: https://git-scm.com/download/win
   - Or use: `winget install Git.Git`

4. **Install vcpkg** (for dependencies)

```powershell
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Add to PATH (run as Administrator)
setx /M PATH "%PATH%;C:\vcpkg"
```

5. **Install dependencies via vcpkg**

```powershell
# Install required libraries (x64)
vcpkg install jansson:x64-windows curl:x64-windows qt6:x64-windows

# Integrate with Visual Studio
vcpkg integrate install
```

### Building

```powershell
# Clone the repository
git clone https://github.com/YOUR_USERNAME/obs-polyemesis.git
cd obs-polyemesis

# Configure the build
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DCMAKE_BUILD_TYPE=Release `
  -DENABLE_FRONTEND_API=ON `
  -DENABLE_QT=ON

# Build the plugin
cmake --build build --config RelWithDebInfo

# The plugin will be located at:
# build\RelWithDebInfo\obs-polyemesis.dll
```

### Installation

```powershell
# Copy to OBS plugins directory
$obsPluginDir = "$env:PROGRAMFILES\obs-studio\obs-plugins\64bit"
Copy-Item build\RelWithDebInfo\obs-polyemesis.dll $obsPluginDir\

# Copy data files
$obsDataDir = "$env:PROGRAMFILES\obs-studio\data\obs-plugins\obs-polyemesis"
New-Item -ItemType Directory -Path $obsDataDir -Force
Copy-Item -Recurse data\* $obsDataDir\
```

### Notes

- Requires Windows 10 or later (64-bit)
- Visual Studio 2019 is also supported
- You can also use Ninja instead of Visual Studio generator

---

## Cross-Platform CI/CD

This project includes GitHub Actions workflows that automatically build for all platforms:

### Supported Platforms

- **macOS**: Universal binary (arm64 + x86_64)
- **Linux**: Ubuntu 24.04 (x86_64)
- **Windows**: x64

### Triggering Builds

Builds are triggered automatically on:
- **Push to main branch**: Full build with codesigning
- **Pull requests**: Build without codesigning
- **Release tags** (e.g., `1.0.0`): Full release build with notarization (macOS)

### Accessing Build Artifacts

1. Go to the **Actions** tab in GitHub
2. Select the workflow run
3. Download artifacts from the bottom of the page

---

## Testing Locally with ACT

[ACT](https://github.com/nektos/act) allows you to run GitHub Actions locally for testing.

### Installing ACT

#### macOS

```bash
brew install act
```

#### Linux

```bash
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash
```

#### Windows

```powershell
choco install act-cli
```

### Running Tests

```bash
# Test the full build workflow
act -j macos-build

# Test specific platforms
act -j ubuntu-build
act -j windows-build

# Test with specific events
act pull_request
act push
```

### Notes

- ACT requires Docker to be installed
- First run will download large container images (~GB)
- macOS builds in ACT may not work perfectly due to Xcode requirements
- Linux and Windows builds work well with ACT

### ACT Configuration

Create `.actrc` in the project root for custom configuration:

```
-P ubuntu-latest=catthehacker/ubuntu:act-latest
-P macos-latest=sickcodes/docker-osx:latest
-P windows-latest=catthehacker/windows:2022
--artifact-server-path /tmp/artifacts
```

---

## Build Options

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_FRONTEND_API` | `ON` | Enable OBS Frontend API (required for UI) |
| `ENABLE_QT` | `ON` | Enable Qt6 support (required for dock panel) |
| `CMAKE_BUILD_TYPE` | - | `Debug`, `Release`, or `RelWithDebInfo` |

### Examples

```bash
# Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Without Qt (no UI dock)
cmake -B build -DENABLE_QT=OFF

# Without Frontend API
cmake -B build -DENABLE_FRONTEND_API=OFF
```

---

## Troubleshooting

### macOS: "AGL framework not found"

**Solution**: The build system automatically uses Homebrew's Qt6. Ensure it's installed:

```bash
brew install qt
```

### Linux: "libobs-dev not found"

**Solution**: Install OBS Studio development files or build OBS from source.

### Windows: "jansson not found"

**Solution**: Install via vcpkg:

```powershell
vcpkg install jansson:x64-windows
```

### Qt6 MOC errors

**Solution**: Ensure Qt6 is properly installed and `CMAKE_PREFIX_PATH` is set:

```bash
# macOS
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt"

# Linux
export CMAKE_PREFIX_PATH="/usr/lib/x86_64-linux-gnu/cmake/Qt6"

# Windows (PowerShell)
$env:CMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
```

---

## Development Tips

### Faster Builds

```bash
# Use Ninja for faster builds (all platforms)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)  # Linux/macOS
cmake --build build -j%NUMBER_OF_PROCESSORS%  # Windows
```

### Incremental Builds

```bash
# After first build, just run:
cmake --build build

# No need to reconfigure unless CMakeLists.txt changes
```

### Clean Build

```bash
# Remove build directory and start fresh
rm -rf build
cmake -B build [options]
cmake --build build
```

---

## Contributing

When submitting pull requests:

1. Ensure code builds on all platforms (CI will verify)
2. Test locally before pushing
3. Update this document if adding new dependencies
4. Follow the existing code style

---

## Support

- **Issues**: https://github.com/YOUR_USERNAME/obs-polyemesis/issues
- **Discussions**: https://github.com/YOUR_USERNAME/obs-polyemesis/discussions
- **Documentation**: See [README.md](../README.md) and [PLUGIN_DOCUMENTATION.md](developer/PLUGIN_DOCUMENTATION.md)
