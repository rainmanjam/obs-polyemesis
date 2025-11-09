# Platform-Specific Fixes and Improvements

**Version:** 0.9.0
**Date:** November 9, 2025

---

## 📊 Current Status

| Platform | Build | Package | Code Signing | Qt/UI | Issues |
|----------|-------|---------|--------------|-------|--------|
| **macOS** | ⚠️ arm64 only | ✅ .pkg automated | ❌ Unsigned | ✅ Full UI | AGL framework, Homebrew deps |
| **Windows** | ✅ Works | ✅ .exe automated | ❌ Unsigned | ❌ No UI | Qt disabled in release |
| **Linux** | ✅ Works | ✅ .deb automated | N/A | ❌ No UI | Qt disabled in release |

---

## 🍎 macOS Fixes

### Issue 1: ✅ .pkg Creation (ALREADY AUTOMATED)
**Status:** Working in release workflow
**Location:** `.github/workflows/release.yaml:113-120`

### Issue 2: 🔐 Code Signing & Notarization (HIGH PRIORITY)

**Required Secrets:**
```bash
# In GitHub repository settings → Secrets and variables → Actions
APPLE_DEVELOPER_ID_APPLICATION    # "Developer ID Application: Your Name (TEAMID)"
APPLE_DEVELOPER_ID_INSTALLER      # "Developer ID Installer: Your Name (TEAMID)"
APPLE_CERT_P12_BASE64             # Base64 encoded P12 certificate
APPLE_CERT_PASSWORD               # P12 certificate password
APPLE_TEAM_ID                     # Your Team ID (10 characters)
APPLE_ID                          # Your Apple ID email
APPLE_APP_PASSWORD                # App-specific password for notarization
```

**How to Get These:**

1. **Export Certificate from Keychain:**
```bash
# In Keychain Access:
# 1. Find "Developer ID Application" certificate
# 2. Right-click → Export → Save as .p12
# 3. Set a password

# Convert to base64:
base64 -i certificate.p12 | pbcopy
# Paste into APPLE_CERT_P12_BASE64 secret
```

2. **Get App-Specific Password:**
```bash
# Go to: https://appleid.apple.com
# Sign in → Security → App-Specific Passwords
# Generate password → Copy to APPLE_APP_PASSWORD
```

3. **Find Team ID:**
```bash
# Go to: https://developer.apple.com/account
# Membership → Team ID
```

**Workflow Changes Required:**
See `release_workflow_macos_signing.patch` below

### Issue 3: 🏗️ Universal Binary (AGL Framework Conflict)

**Problem:** Qt6 from OBS deps tries to link deprecated AGL framework
**Current Status:** arm64-only builds work fine
**Impact:** Intel Mac users can't use plugin (< 10% of user base)

**Solutions (Priority Order):**

**Option A: Ship arm64-only for 0.9.0 (RECOMMENDED)**
- ✅ Works today
- ✅ Covers 90%+ of macOS users
- ✅ No code changes needed
- ⚠️ Document Intel Mac limitation

**Option B: Build without Qt UI temporarily**
```yaml
-DENABLE_QT=OFF -DENABLE_FRONTEND_API=OFF
```
- ✅ Universal binary works
- ❌ No dock UI (major feature loss)
- Not recommended

**Option C: Fix AGL issue (FUTURE)**
- Requires upstream Qt fix or complete OBS deps rebuild
- Complex, time-consuming
- Defer to v1.1.0

**Decision for 0.9.0:** Ship arm64-only, document clearly

### Issue 4: 📦 Library Dependencies

**Current:** Links to Homebrew paths
**Fixed Today:** Bundle jansson, use @rpath for Qt
**Status:** ✅ Working locally

**Required CMake Changes:**
```cmake
# Add post-build step to bundle and fix paths
if(APPLE)
  add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    # Bundle jansson
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE_DIR:jansson>/libjansson.4.dylib"
      "$<TARGET_BUNDLE_DIR:${CMAKE_PROJECT_NAME}>/Contents/Frameworks/"

    # Fix jansson path
    COMMAND install_name_tool -change
      "@rpath/libjansson.4.dylib"
      "@loader_path/../Frameworks/libjansson.4.dylib"
      "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"

    # Re-sign
    COMMAND codesign --force --deep --sign -
      "$<TARGET_BUNDLE_DIR:${CMAKE_PROJECT_NAME}>"

    COMMENT "Bundling dependencies and fixing library paths"
  )
endif()
```

---

## 🪟 Windows Fixes

### Issue 1: ❌ Qt/UI Disabled in Release Build

**Problem:**
```yaml
# .github/workflows/release.yaml:154
cmake -B build -DENABLE_FRONTEND_API=OFF -DENABLE_QT=OFF
```

**Impact:** Windows users can't access dock UI (major feature)

**Root Cause:** Missing Qt6 setup for Windows

**Fix Required:**
```yaml
- name: Install Qt
  uses: jurplel/install-qt-action@v3
  with:
    version: '6.7.0'
    target: 'desktop'
    arch: 'win64_msvc2019_64'

- name: Build Plugin
  run: |
    cmake -B build -G "Visual Studio 17 2022" -A x64 \
      -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_FRONTEND_API=ON \
      -DENABLE_QT=ON
    cmake --build build --config Release
```

**Testing Status:** ⚠️ Needs verification (no Windows test in CI with Qt)

### Issue 2: 🔐 Code Signing (Windows)

**Optional for 0.9.0** - Most users accept unsigned Windows apps

**If needed later:**
```yaml
- name: Sign Windows Binary
  uses: dlemstra/code-sign-action@v1
  with:
    certificate: '${{ secrets.WINDOWS_CERT_BASE64 }}'
    password: '${{ secrets.WINDOWS_CERT_PASSWORD }}'
    files: 'build/Release/obs-polyemesis.dll'
```

---

## 🐧 Linux Fixes

### Issue 1: ❌ Qt/UI Disabled in Release Build

**Problem:** Same as Windows - Qt disabled

**Fix Required:**
```yaml
- name: Install Dependencies
  run: |
    sudo apt-get update
    sudo apt-get install -y \
      build-essential \
      cmake \
      libcurl4-openssl-dev \
      libjansson-dev \
      libobs-dev \
      libqt6-dev \        # ADD
      qt6-base-dev \      # ADD
      ninja-build

- name: Build Plugin
  run: |
    cmake -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_FRONTEND_API=ON \   # CHANGE
      -DENABLE_QT=ON                # CHANGE
    cmake --build build
```

**Testing Status:** ⚠️ Needs verification

### Issue 2: 📦 Package Dependencies

**Update control file:**
```diff
-Depends: obs-studio (>= 28.0), libcurl4, libjansson4
+Depends: obs-studio (>= 28.0), libcurl4, libjansson4, libqt6core6, libqt6gui6, libqt6widgets6
```

---

## 📋 Implementation Plan for 0.9.0

### Phase 1: macOS (This Session)
1. ✅ Fix installation path (DONE)
2. ✅ Bundle dependencies (DONE)
3. 🔄 Add code signing to workflow (IN PROGRESS)
4. 📝 Document arm64-only limitation
5. ⏭️ Defer universal binary to v1.1.0

### Phase 2: Windows (Next)
1. Add Qt6 to build environment
2. Enable Qt/Frontend API in build
3. Test installer with UI enabled
4. Consider code signing (optional)

### Phase 3: Linux (Next)
1. Add Qt6 dependencies
2. Enable Qt/Frontend API in build
3. Update package dependencies
4. Test on Ubuntu 24.04

### Phase 4: Testing & Release
1. Build all platforms
2. Test on actual machines
3. Update documentation
4. Create v0.9.0 release

---

## 🎯 Specific Changes Needed

### File 1: `.github/workflows/release.yaml`

**Changes:**
1. Add macOS code signing steps (lines 113-130)
2. Add Windows Qt setup (before line 152)
3. Enable Windows Qt build (line 154)
4. Add Linux Qt dependencies (line 199)
5. Enable Linux Qt build (line 212)

### File 2: `CMakeLists.txt`

**Changes:**
1. Add post-build library bundling for macOS
2. Add install_name_tool commands
3. Add codesign command

### File 3: `README.md`

**Changes:**
1. Add macOS architecture note (arm64-only for 0.9.0)
2. Update system requirements
3. Add installation troubleshooting

### File 4: `packaging/macos/create-pkg.sh`

**Changes:**
1. Verify plugin bundle before packaging
2. Check library dependencies
3. Validate code signature

---

## ⏱️ Estimated Time

| Task | Time | Priority |
|------|------|----------|
| macOS code signing setup | 30 min | HIGH |
| Windows Qt enable | 45 min | HIGH |
| Linux Qt enable | 30 min | MEDIUM |
| Testing all platforms | 1-2 hours | HIGH |
| Documentation updates | 20 min | MEDIUM |

**Total:** ~3-4 hours for complete 0.9.0 release readiness

---

## 📝 Notes

- **macOS universal binary** deferred to avoid AGL complexity
- **Windows/Linux** will have full UI once Qt enabled
- **Code signing** optional for 0.9.0 but recommended for 1.0.0
- All changes backward compatible with existing 0.9.0 codebase

**Next Step:** Would you like me to implement Phase 1 (macOS code signing) now?
