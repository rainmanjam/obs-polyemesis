# Quick Start Guide - OBS Polyemesis Development

Quick reference for building and testing OBS Polyemesis across all platforms.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Commands](#quick-commands)
- [Platform-Specific Building](#platform-specific-building)
- [Testing](#testing)
- [Pre-Commit Checks](#pre-commit-checks)
- [Common Workflows](#common-workflows)

---

## Prerequisites

### macOS
```bash
# Install dependencies
brew install cmake ninja qt6 jansson curl pkg-config
brew install --cask obs

# Optional: Install development tools
brew install shellcheck act
```

### Linux (via Docker/act)
```bash
# Install act for local CI/CD testing
brew install act  # macOS
# or
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash  # Linux
```

### Windows
See [Windows Testing Guide](docs/developer/WINDOWS_TESTING.md) for SSH setup.

---

## Quick Commands

### Using Make (Recommended)

```bash
# Show all available commands
make help

# Build for current platform (macOS)
make build

# Build for all platforms
make build-all

# Run tests
make test

# Run tests on all platforms
make test-all

# Create installer package
make package

# Clean build artifacts
make clean

# Install pre-commit hooks
make pre-commit-install
```

### Using Scripts Directly

```bash
# macOS
./scripts/macos-build.sh
./scripts/macos-test.sh
./scripts/macos-package.sh

# Linux (via Docker)
act -j build-linux
act -W .github/workflows/test.yaml

# Windows (via SSH)
./scripts/sync-and-build-windows.sh
./scripts/windows-test.sh
./scripts/windows-package.sh

# All platforms
./scripts/build-all-platforms.sh
./scripts/test-all-platforms.sh
```

---

## Platform-Specific Building

### macOS

```bash
# Standard universal binary (ARM + Intel)
./scripts/macos-build.sh

# ARM64 only (Apple Silicon)
./scripts/macos-build.sh --arch arm64

# Debug build
./scripts/macos-build.sh --build-type Debug

# With tests enabled
./scripts/macos-build.sh --enable-tests

# Clean rebuild
./scripts/macos-build.sh --clean
```

**Build output:**
- Plugin: `build/Release/obs-polyemesis.so`
- Tests: `build/tests/Release/obs-polyemesis-tests`

### Linux (Docker)

```bash
# Build using GitHub Actions workflow locally
act -W .github/workflows/create-packages.yaml -j build-linux

# With verbose output
act -W .github/workflows/create-packages.yaml -j build-linux --verbose

# Run tests
act -W .github/workflows/test.yaml

# Specific test job
act -j unit-tests
```

**Build output:** Inside Docker container (act cache)

### Windows (Remote SSH)

```bash
# Sync and build
./scripts/sync-and-build-windows.sh

# Sync only (no build)
./scripts/sync-and-build-windows.sh --sync-only

# Build with custom workflow
./scripts/sync-and-build-windows.sh --workflow .github/workflows/custom.yaml
```

**Build output:** On Windows machine at `C:\Users\rainm\Documents\GitHub\obs-polyemesis\build\`

---

## Testing

### Run Tests

```bash
# macOS
make test-macos
# or
./scripts/macos-test.sh

# Build and test
./scripts/macos-test.sh --build-first

# Run specific test
./scripts/macos-test.sh --filter "restreamer.*"

# Linux
make test-linux
# or
act -W .github/workflows/test.yaml

# Windows
make test-windows
# or
./scripts/windows-test.sh

# All platforms
make test-all
# or
./scripts/test-all-platforms.sh
```

### Pre-Commit Checks

```bash
# Install hooks (run once)
make pre-commit-install

# Run checks manually
make pre-commit
# or
./scripts/pre-commit

# Individual checks
make syntax-check  # Bash syntax
make lint          # ShellCheck
make format        # Code formatting
```

---

## Common Workflows

### 1. Local Development (macOS)

```bash
# 1. Make changes to code
vim src/plugin-main.c

# 2. Build
make build

# 3. Test
make test

# 4. Install to OBS for testing
make install

# 5. Test in OBS
open /Applications/OBS.app

# 6. Uninstall when done
make uninstall
```

### 2. Pre-Push Validation

```bash
# Test locally before pushing to GitHub
make build-all    # Build all platforms
make test-all     # Test all platforms

# Or use quick validation
make check        # Syntax and lint checks
make quick-test   # Fast macOS test
```

### 3. Release Build

```bash
# Build and package for all platforms
make release

# Artifacts will be in:
# - build/installers/*.pkg (macOS)
# - Windows: Fetch with ./scripts/windows-package.sh --fetch
```

### 4. Debug Build and Test

```bash
# macOS debug build
BUILD_TYPE=Debug make build

# Or
make quick-build  # Shortcut for debug build

# Run with debugger
lldb build/Debug/obs-polyemesis.so
```

### 5. Clean Slate

```bash
# Clean current platform
make clean

# Clean all platforms
make clean-all

# Rebuild from scratch
make rebuild  # clean + build
```

---

## Platform Testing Matrix

| Platform | Build Location | Command | Output Location |
|----------|---------------|---------|-----------------|
| **macOS** | Local | `make build-macos` | `build/Release/obs-polyemesis.so` |
| **Linux** | Docker (act) | `make build-linux` | Docker cache |
| **Windows** | Remote SSH | `make build-windows` | Windows: `C:\Users\rainm\Documents\GitHub\obs-polyemesis\build\` |

---

## Troubleshooting

### Build Failures

```bash
# Check dependencies
make deps-check

# View build configuration
make info

# Clean and rebuild
make rebuild
```

### Test Failures

```bash
# Run with verbose output
VERBOSE=1 make test

# Test specific component
./scripts/macos-test.sh --filter "api.*"
```

### Windows Connection Issues

```bash
# Test SSH connection
ssh windows-act

# Check Windows machine status
./scripts/windows-act.sh -l
```

---

## Environment Variables

```bash
# Build configuration
export BUILD_TYPE=Debug        # or Release, RelWithDebInfo
export BUILD_DIR=build        # Custom build directory
export VERBOSE=1               # Enable verbose output

# Platform specific
export MACOS_ARCHITECTURES="arm64;x86_64"  # Universal binary
export WINDOWS_ACT_HOST=windows-act        # Windows SSH host
export WINDOWS_WORKSPACE="C:/Users/rainm/Documents/GitHub/obs-polyemesis"

# Example: Debug build with verbose output
BUILD_TYPE=Debug VERBOSE=1 make build
```

---

## Additional Resources

- **[Windows Testing Guide](docs/developer/WINDOWS_TESTING.md)** - Remote Windows testing setup
- **[Act Testing Guide](docs/developer/ACT_TESTING.md)** - Local CI/CD with Docker
- **[Contributing Guide](CONTRIBUTING.md)** - Full contribution guidelines
- **[Makefile](Makefile)** - Complete command reference (`make help`)

---

## Quick Decision Tree

**Want to build locally?**
- macOS → `make build`
- Linux → `act -j build-linux`
- Windows → `./scripts/sync-and-build-windows.sh`

**Want to test locally?**
- macOS → `make test`
- Linux → `act -W .github/workflows/test.yaml`
- Windows → `./scripts/windows-test.sh`

**Want to test everything before pushing?**
```bash
make build-all && make test-all
```

**Ready to create a release?**
```bash
make release
```

---

Generated for OBS Polyemesis local testing workflow.
