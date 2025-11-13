# OBS Polyemesis - Build Matrix & Platform Support

## Supported Platforms

| Platform | Architecture | Status | Installer Type | Minimum OBS Version |
|----------|--------------|--------|----------------|---------------------|
| **macOS** | Universal (arm64 + x86_64) | ✅ Supported | `.pkg` | 30.0 |
| **Windows** | x64 (64-bit) | ✅ Supported | `.exe` | 30.0 |
| **Linux** | x86_64 (64-bit) | ✅ Supported | `.deb`, `.rpm` | 30.0 |
| **Linux** | aarch64 (ARM64) | ✅ Supported | `.deb`, `.rpm` | 30.0 |

## Build Commands

### macOS Universal Binary (arm64 + x86_64)

```bash
# Configure
cmake -B .build-universal -G Xcode \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DENABLE_FRONTEND_API=ON \
  -DENABLE_QT=ON

# Build
cmake --build .build-universal --config Release

# Verify
lipo -info .build-universal/Release/obs-polyemesis.plugin/Contents/MacOS/obs-polyemesis
# Expected: Architectures in the fat file: arm64 x86_64
```

### Linux x86_64

```bash
# Configure
cmake -B .build-x86_64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_FRONTEND_API=ON \
  -DENABLE_QT=ON

# Build
cmake --build .build-x86_64

# Verify
file .build-x86_64/obs-polyemesis.so
# Expected: ELF 64-bit LSB shared object, x86-64
```

### Linux ARM64 (aarch64)

```bash
# Configure
cmake -B .build-aarch64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_FRONTEND_API=ON \
  -DENABLE_QT=ON

# Build
cmake --build .build-aarch64

# Verify
file .build-aarch64/obs-polyemesis.so
# Expected: ELF 64-bit LSB shared object, ARM aarch64
```

### Windows x64

```powershell
# Configure
cmake -B .build-x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DENABLE_FRONTEND_API=ON `
  -DENABLE_QT=ON

# Build
cmake --build .build-x64 --config Release

# Verify
dumpbin /headers .build-x64\Release\obs-polyemesis.dll | findstr machine
# Expected: machine (x64)
```

## Platform-Specific Notes

### macOS Universal Binary

**Key Features:**
- Single binary works on both Intel Macs and Apple Silicon
- Automatically builds jansson from source as universal library
- Static linking adds ~50KB to binary size
- Eliminates Homebrew architecture dependency issues

**Dependencies:**
- Xcode 16.1+ with Command Line Tools
- OBS Studio 30.0+ (bundles Qt6)

**Installation:**
```bash
# Via package manager
cp -r obs-polyemesis.plugin ~/Library/Application\ Support/obs-studio/plugins/

# Verify installation
ls ~/Library/Application\ Support/obs-studio/plugins/obs-polyemesis.plugin/Contents/MacOS/
```

### Linux (x86_64 and ARM64)

**Key Features:**
- Native builds for both x86_64 and ARM64
- All dependencies available from Ubuntu 24.04 repositories
- No code changes required between architectures
- Binary size: ~147KB

**Dependencies:**
```bash
# Ubuntu/Debian
sudo apt install \
  libobs-dev \
  obs-frontend-api \
  libcurl4-openssl-dev \
  libjansson-dev \
  qt6-base-dev

# Fedora/RHEL
sudo dnf install \
  obs-studio-devel \
  libcurl-devel \
  jansson-devel \
  qt6-qtbase-devel
```

**Installation:**
```bash
# Via package manager
sudo dpkg -i obs-polyemesis_*.deb  # Debian/Ubuntu
sudo rpm -i obs-polyemesis-*.rpm   # Fedora/RHEL

# Manual installation
cp obs-polyemesis.so ~/.config/obs-studio/plugins/obs-polyemesis/bin/64bit/
```

### Windows (x64)

**Key Features:**
- Native 64-bit Windows binary
- Includes all required dependencies

**Dependencies:**
- Visual Studio 2022 (for building)
- OBS Studio 30.0+ (provides Qt6)

**Installation:**
```powershell
# Run the installer
obs-polyemesis-*-windows-x64.exe

# Or manual copy
Copy-Item obs-polyemesis.dll "$env:ProgramFiles\obs-studio\obs-plugins\64bit\"
Copy-Item data "$env:ProgramFiles\obs-studio\data\obs-plugins\obs-polyemesis\" -Recurse
```

## CI/CD Build Matrix

GitHub Actions automatically builds for all supported platforms:

```yaml
jobs:
  macos-build:
    runs-on: macos-15
    target: macos-universal  # arm64 + x86_64

  ubuntu-build:
    runs-on: ubuntu-24.04
    strategy:
      matrix:
        arch: [x86_64, aarch64]

  windows-build:
    runs-on: windows-2022
    target: x64
```

## Architecture Detection

The plugin automatically detects the build architecture:

```cmake
# macOS universal binary detection
if(APPLE AND CMAKE_OSX_ARCHITECTURES MATCHES ".*;.*")
  # Build jansson from source as universal library
  include(cmake/BuildJanssonUniversal.cmake)
endif()
```

At runtime, OBS loads the appropriate architecture:
- **macOS**: Automatically selects arm64 or x86_64 based on running processor
- **Linux**: Uses native architecture binary (x86_64 or aarch64)
- **Windows**: Uses x64 binary

## Target Hardware

### macOS Universal
- **Apple Silicon**: M1, M2, M3, M4 (native arm64)
- **Intel Macs**: All 64-bit Intel processors (native x86_64)

### Linux x86_64
- Desktop PCs with x86_64 processors
- Servers and workstations
- Intel and AMD 64-bit CPUs

### Linux ARM64
- **Raspberry Pi**: Pi 4, Pi 5, Pi 400 (4GB+ recommended)
- **ARM Servers**: AWS Graviton, Ampere Altra, etc.
- **SBCs**: NVIDIA Jetson, Odroid, Rock Pi, etc.
- **Apple Silicon Linux VMs**: Parallels, VMware Fusion, UTM

### Windows x64
- All modern Windows PCs (Windows 10/11 64-bit)
- Intel and AMD 64-bit processors

## Binary Sizes

Approximate compiled binary sizes:

| Platform | Architecture | Size | Notes |
|----------|--------------|------|-------|
| macOS | Universal | ~450KB | Includes both arm64 + x86_64 |
| Linux | x86_64 | ~150KB | Dynamically linked |
| Linux | aarch64 | ~147KB | Dynamically linked |
| Windows | x64 | ~200KB | Dynamically linked |

## Future Platform Support

Platforms under consideration for future releases:

- **FreeBSD x86_64**: Feasible, similar to Linux
- **OpenBSD x86_64**: Feasible, similar to Linux
- **Windows ARM64**: Waiting for OBS Studio official support

## Testing

All platforms are tested in CI:

```bash
# Run cross-platform tests locally with act
act -j macos-build     # Test macOS universal build
act -j ubuntu-build    # Test Linux x86_64 and ARM64 builds
act -j windows-build   # Test Windows x64 build
```

## Support Matrix

| Feature | macOS | Windows | Linux x64 | Linux ARM64 |
|---------|-------|---------|-----------|-------------|
| Restreamer API v3 | ✅ | ✅ | ✅ | ✅ |
| JWT Authentication | ✅ | ✅ | ✅ | ✅ |
| Multistream Profiles | ✅ | ✅ | ✅ | ✅ |
| OBS Bridge (RTMP) | ✅ | ✅ | ✅ | ✅ |
| Process Monitoring | ✅ | ✅ | ✅ | ✅ |
| Native UI Theming | ✅ | ✅ | ✅ | ✅ |
| Collapsible Sections | ✅ | ✅ | ✅ | ✅ |

---

**Last Updated:** 2025-11-12
**Plugin Version:** 0.9.0
**Minimum OBS Version:** 30.0
