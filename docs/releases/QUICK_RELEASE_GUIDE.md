# Quick Release Guide - 0.9.0

**Ready to ship?** Follow these steps to release v0.9.0

---

## Option A: Release WITHOUT Code Signing (Fastest)

**Time:** 5 minutes
**Result:** Working packages for all platforms (unsigned)

```bash
# 1. Commit changes
git add .
git commit -m "Release v0.9.0 - Multi-platform UI fixes and improvements"
git push origin main

# 2. Create and push release tag
git tag -a v0.9.0 -m "Release v0.9.0"
git push origin v0.9.0

# 3. Done!
# GitHub Actions will automatically:
# - Build for macOS (arm64), Windows (x64), Linux (amd64)
# - Create .pkg, .exe, and .deb packages
# - Upload to GitHub Releases
# - Generate checksums
```

**Monitor progress:** https://github.com/rainmanjam/obs-polyemesis/actions

**Note:** macOS users will see security warning (workaround documented in README)

---

## Option B: Release WITH Code Signing (Recommended)

**Time:** 20-25 minutes (first time setup)
**Result:** Professional signed & notarized macOS packages

### Step 1: Set Up Apple Developer Secrets (One-time)

Follow **`APPLE_CODE_SIGNING_SETUP.md`** to add 7 secrets to GitHub:

1. `APPLE_CERT_P12_BASE64`
2. `APPLE_CERT_PASSWORD`
3. `APPLE_DEVELOPER_ID_APPLICATION`
4. `APPLE_DEVELOPER_ID_INSTALLER`
5. `APPLE_TEAM_ID`
6. `APPLE_ID`
7. `APPLE_APP_PASSWORD`

**Guide:** `APPLE_CODE_SIGNING_SETUP.md` (step-by-step with examples)

### Step 2: Release (Same as Option A)

```bash
git add .
git commit -m "Release v0.9.0 - Multi-platform UI fixes and improvements"
git push origin main

git tag -a v0.9.0 -m "Release v0.9.0"
git push origin v0.9.0
```

**Result:** macOS package will be **signed AND notarized** - no security warnings!

---

## What Gets Built

When you push the tag, GitHub Actions builds:

| Platform | File | Size | Qt UI |
|----------|------|------|-------|
| macOS (arm64) | `obs-polyemesis-0.9.0-macos.pkg` | ~150KB | ✅ |
| Windows (x64) | `obs-polyemesis-0.9.0-windows-x64.exe` | ~50KB | ✅ |
| Linux (amd64) | `obs-polyemesis_0.9.0_amd64.deb` | ~40KB | ✅ |

Plus: `SHA256SUMS.txt` with checksums

---

## Monitoring the Build

### 1. Watch GitHub Actions
https://github.com/rainmanjam/obs-polyemesis/actions

### 2. Check Build Jobs
- ✅ `create-release` - Creates GitHub release
- ✅ `build-macos` - macOS package (5-10 min)
- ✅ `build-windows` - Windows installer (3-5 min)
- ✅ `build-linux` - Linux package (2-3 min)
- ✅ `update-checksums` - SHA256 sums
- ✅ `notify` - Completion status

### 3. If Code Signing Enabled
Look for these additional steps in `build-macos`:
- ✅ Import Code Signing Certificate
- ✅ Sign Plugin Bundle
- ✅ Sign Package
- ✅ Notarize Package (~2-5 min wait for Apple)

---

## After Build Completes

### 1. Download & Test Packages

Download from: https://github.com/rainmanjam/obs-polyemesis/releases/tag/v0.9.0

**macOS:**
```bash
# Download and install
open obs-polyemesis-0.9.0-macos.pkg

# Or if unsigned:
# Right-click → Open → Open
```

**Windows:**
```powershell
# Download and run
.\obs-polyemesis-0.9.0-windows-x64.exe
```

**Linux:**
```bash
# Download and install
sudo dpkg -i obs-polyemesis_0.9.0_amd64.deb
```

### 2. Test in OBS Studio

1. Open OBS Studio
2. Check: **Tools** → **Plugins** (should show "obs-polyemesis")
3. Check: **View** → **Docks** → **Restreamer Control** (should open dock)
4. Test: Connect to Restreamer instance

---

## If Something Goes Wrong

### Build Fails - macOS
**Check:**
- Is Qt6 available? (Homebrew install)
- Are library paths correct?
- If signing fails, check secret names match exactly

**Quick Fix:**
```bash
# Delete tag and try again
git tag -d v0.9.0
git push origin :refs/tags/v0.9.0
# Fix issue, then re-tag
```

### Build Fails - Windows
**Check:**
- Qt installation step succeeded?
- Visual Studio 2022 available?

### Build Fails - Linux
**Check:**
- Qt6 packages available on Ubuntu 24.04?
- libobs-dev installed?

### Certificate Issues (Code Signing)
**Check:**
1. All 7 secrets added to GitHub?
2. Identity strings exact match? (check with `security find-identity`)
3. P12 password correct?
4. Apple Developer Program active?

**Quick Fix:**
- Remove secrets temporarily to release unsigned
- Fix secrets later and re-release with `-signed` suffix

---

## Post-Release Checklist

After successful release:

### Update Documentation
- [ ] Add release link to README.md
- [ ] Update installation instructions
- [ ] Add v0.9.0 known limitations (arm64-only macOS)

### Announce
- [ ] GitHub Discussions post
- [ ] Update project description
- [ ] Post to promotional sites (see earlier promotional posts)

### Monitor
- [ ] Watch for GitHub Issues
- [ ] Check installation reports
- [ ] Gather feedback

---

## Quick Commands Reference

```bash
# Check current version
grep '"version"' buildspec.json

# View recent tags
git tag -l

# View commit history
git log --oneline -10

# Check GitHub Actions status
gh run list --limit 5

# Download release locally
gh release download v0.9.0

# Delete a release (if needed)
gh release delete v0.9.0 --yes
git tag -d v0.9.0
git push origin :refs/tags/v0.9.0
```

---

## Expected Timeline

### Without Code Signing
- **Commit & Push:** 1 min
- **Tag & Push:** 1 min
- **Build All Platforms:** 10-15 min
- **Total:** ~15-20 minutes

### With Code Signing (First Time)
- **Setup Secrets:** 15-20 min (one-time)
- **Commit & Push:** 1 min
- **Tag & Push:** 1 min
- **Build + Sign + Notarize:** 15-20 min
- **Total:** ~35-40 minutes (first time)
- **Future Releases:** ~20 minutes

---

## TL;DR - Absolute Minimum

```bash
# Literally just these 4 commands:
git add .
git commit -m "Release v0.9.0"
git tag -a v0.9.0 -m "Release v0.9.0"
git push origin main v0.9.0

# Wait 15 minutes
# Download packages from GitHub Releases
# Done! 🎉
```

---

## Need Help?

- **Build Logs:** GitHub Actions → Click workflow → Click job → View logs
- **Secrets:** Settings → Secrets and variables → Actions
- **Releases:** https://github.com/rainmanjam/obs-polyemesis/releases
- **Documentation:** All guides in repo root (*.md files)

---

**Ready?** Choose Option A or B above and ship it! 🚀

**Questions?** Check:
1. `RELEASE_READY_SUMMARY.md` - Comprehensive overview
2. `APPLE_CODE_SIGNING_SETUP.md` - Certificate setup
3. `PLATFORM_FIXES.md` - Technical details
4. `COMPLIANCE_REVIEW.md` - OBS plugin compliance
