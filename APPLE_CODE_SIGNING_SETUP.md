# Apple Code Signing & Notarization Setup Guide

**Project:** OBS Polyemesis
**Date:** November 9, 2025

This guide walks you through setting up code signing and notarization for automatic macOS package signing in GitHub Actions.

---

## Prerequisites

- ✅ Active Apple Developer Account ($99/year)
- ✅ Access to Keychain Access on macOS
- ✅ Admin access to GitHub repository settings

---

## Step 1: Export Your Developer ID Certificates

### 1.1 Open Keychain Access
```bash
open /Applications/Utilities/Keychain\ Access.app
```

### 1.2 Find Your Certificates

Look for these two certificates in the "login" keychain under "My Certificates":
1. **Developer ID Application: Your Name (TEAM_ID)**
2. **Developer ID Installer: Your Name (TEAM_ID)**

> If you don't see these, you need to create them in your [Apple Developer Account](https://developer.apple.com/account/resources/certificates).

### 1.3 Export as P12

**For BOTH certificates:**

1. Select the certificate (make sure to expand and select the certificate WITH the key)
2. Right-click → **Export "Developer ID..."**
3. Save as: `developer-id-application.p12` (or `-installer.p12`)
4. **Set a strong password** (you'll need this later)
5. Click **Save**
6. Enter your macOS password when prompted

You should now have:
- `developer-id-application.p12`
- `developer-id-installer.p12`

---

## Step 2: Convert Certificates to Base64

### 2.1 Convert Application Certificate
```bash
base64 -i developer-id-application.p12 | pbcopy
```
✅ The base64 string is now in your clipboard

### 2.2 Convert Installer Certificate
```bash
base64 -i developer-id-installer.p12 | pbcopy
```
✅ The base64 string is now in your clipboard

---

## Step 3: Get Your Apple Team ID

### Method 1: From Developer Portal
1. Go to https://developer.apple.com/account
2. Click **Membership** in the sidebar
3. Your Team ID is displayed (10 characters, e.g., `AB1234CDEF`)

### Method 2: From Certificate
```bash
security find-identity -v -p codesigning
```
Look for your Developer ID certificate - the Team ID is in parentheses.

---

## Step 4: Create App-Specific Password

### 4.1 Generate Password
1. Go to https://appleid.apple.com
2. Sign in with your Apple ID
3. Go to **Security** section
4. Under **App-Specific Passwords**, click **Generate Password**
5. Label it: `GitHub Actions - OBS Polyemesis`
6. Click **Create**
7. **Copy the password** (you can't see it again!)

Format: `xxxx-xxxx-xxxx-xxxx`

---

## Step 5: Add Secrets to GitHub

### 5.1 Navigate to Repository Settings
1. Go to https://github.com/rainmanjam/obs-polyemesis
2. Click **Settings** tab
3. Click **Secrets and variables** → **Actions**
4. Click **New repository secret**

### 5.2 Add Each Secret

Create these secrets one by one:

#### Secret 1: APPLE_CERT_P12_BASE64
- **Name:** `APPLE_CERT_P12_BASE64`
- **Value:** Paste the base64 string from Step 2.1
- Click **Add secret**

#### Secret 2: APPLE_CERT_PASSWORD
- **Name:** `APPLE_CERT_PASSWORD`
- **Value:** The password you set when exporting the P12 (same for both certs)
- Click **Add secret**

#### Secret 3: APPLE_DEVELOPER_ID_APPLICATION
- **Name:** `APPLE_DEVELOPER_ID_APPLICATION`
- **Value:** `Developer ID Application: Your Name (TEAM_ID)`
  - Example: `Developer ID Application: John Doe (AB1234CDEF)`
  - Find exact string with: `security find-identity -v -p codesigning`
- Click **Add secret**

#### Secret 4: APPLE_DEVELOPER_ID_INSTALLER
- **Name:** `APPLE_DEVELOPER_ID_INSTALLER`
- **Value:** `Developer ID Installer: Your Name (TEAM_ID)`
  - Example: `Developer ID Installer: John Doe (AB1234CDEF)`
- Click **Add secret**

#### Secret 5: APPLE_TEAM_ID
- **Name:** `APPLE_TEAM_ID`
- **Value:** Your Team ID from Step 3
  - Example: `AB1234CDEF`
- Click **Add secret**

#### Secret 6: APPLE_ID
- **Name:** `APPLE_ID`
- **Value:** Your Apple ID email
  - Example: `your.email@example.com`
- Click **Add secret**

#### Secret 7: APPLE_APP_PASSWORD
- **Name:** `APPLE_APP_PASSWORD`
- **Value:** The app-specific password from Step 4
  - Example: `xxxx-xxxx-xxxx-xxxx`
- Click **Add secret**

---

## Step 6: Verify Secrets

After adding all secrets, you should see:

```
✅ APPLE_CERT_P12_BASE64
✅ APPLE_CERT_PASSWORD
✅ APPLE_DEVELOPER_ID_APPLICATION
✅ APPLE_DEVELOPER_ID_INSTALLER
✅ APPLE_TEAM_ID
✅ APPLE_ID
✅ APPLE_APP_PASSWORD
```

---

## Step 7: Test the Workflow

### 7.1 Trigger a Release
```bash
# Create a test tag
git tag -a v0.9.0-test -m "Test release for code signing"
git push origin v0.9.0-test
```

### 7.2 Monitor the Workflow
1. Go to **Actions** tab
2. Click on the running **Release** workflow
3. Watch the **build-macos** job
4. Check these steps:
   - ✅ Import Code Signing Certificate
   - ✅ Sign Plugin Bundle
   - ✅ Sign Package
   - ✅ Notarize Package

### 7.3 Verify Success
If all steps pass:
- ✅ Plugin bundle is signed
- ✅ Package (.pkg) is signed
- ✅ Package is notarized by Apple
- ✅ Users won't see security warnings!

---

## Troubleshooting

### Issue: "Certificate not found"
**Solution:** Verify the identity string exactly matches:
```bash
security find-identity -v -p codesigning
```
Copy the EXACT string including spaces and parentheses.

### Issue: "Notarization failed"
**Solutions:**
1. Verify Apple ID and App-Specific Password are correct
2. Check Team ID matches your account
3. Ensure you're using app-specific password (NOT your main Apple ID password)
4. Check Apple Developer Program is active

### Issue: "Base64 decode failed"
**Solutions:**
1. Re-export P12 certificate
2. Ensure base64 command completed successfully
3. Don't add extra characters when pasting

### Issue: "Keychain unlock failed"
**Solutions:**
1. Verify APPLE_CERT_PASSWORD is correct
2. Ensure P12 file was exported with the same password
3. Try exporting P12 again with a simple password for testing

---

## Security Best Practices

### ✅ DO:
- Use strong, unique password for P12 files
- Store P12 files securely offline after setup
- Rotate app-specific passwords annually
- Use GitHub secret scanning
- Limit repository access

### ❌ DON'T:
- Commit P12 files to repository
- Share certificates between projects unnecessarily
- Use main Apple ID password for automation
- Leave certificates in Downloads folder

---

## Cleanup After Setup

### Delete Local Certificates
```bash
# After confirming secrets work
rm developer-id-application.p12
rm developer-id-installer.p12

# Clear clipboard
pbcopy < /dev/null
```

### Secure Backup (Optional)
If you want to keep backups:
```bash
# Encrypt and backup
openssl enc -aes-256-cbc -salt \
  -in developer-id-application.p12 \
  -out developer-id-application.p12.enc

# Store .enc files in secure location (NOT in git repo)
```

---

## What Happens During Release

When you push a tag like `v0.9.0`:

1. **GitHub Actions starts** release workflow
2. **Builds** plugin for arm64 macOS
3. **Bundles** jansson dependency
4. **Signs** plugin bundle with Developer ID Application
5. **Creates** .pkg installer
6. **Signs** .pkg with Developer ID Installer
7. **Submits** to Apple for notarization
8. **Waits** for Apple approval (~2-5 minutes)
9. **Staples** notarization ticket to package
10. **Uploads** signed & notarized package to release

**Result:** Users can install without any security warnings! ✅

---

## Cost Analysis

| Item | Cost | Frequency |
|------|------|-----------|
| Apple Developer Program | $99 | Annually |
| Code Signing | Free | Per release |
| Notarization | Free | Per release |
| GitHub Actions | Free | 2000 min/month |

**Total:** $99/year for professional distribution

---

## Need Help?

- **Apple Developer Support:** https://developer.apple.com/support/
- **Notarization Guide:** https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution
- **GitHub Actions:** https://docs.github.com/en/actions

---

## Next Steps After Setup

1. ✅ Commit workflow changes
2. ✅ Push to GitHub
3. ✅ Create release tag
4. ✅ Verify signed package downloads
5. ✅ Update README with installation notes
6. 🎉 Ship to users!

---

**Setup Time:** ~15-20 minutes
**One-time setup:** Yes (renew annually)
**Worth it:** Absolutely! ✅
