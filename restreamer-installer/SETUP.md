# Setting Up the Restreamer Installer Repository

This document explains how to publish the Restreamer installer to a separate GitHub repository for public use.

## Files Created

The following files have been created in the `restreamer-installer/` directory:

### Core Scripts
- **install.sh** - Main installation script (one-line installer)
- **uninstall.sh** - Complete uninstallation script
- **diagnose.sh** - Diagnostic and troubleshooting tool

### Documentation
- **README.md** - Complete documentation with installation instructions
- **QUICKSTART.md** - Quick start guide for new users
- **SETUP.md** - This file (repository setup instructions)

### CI/CD
- **.github/workflows/test-install.yml** - GitHub Actions workflow for testing

## Next Steps

### 1. Create a New GitHub Repository

```bash
# Create a new repository on GitHub named "restreamer-installer"
# Then, from the restreamer-installer directory:

cd restreamer-installer
git init
git add .
git commit -m "Initial commit: Polyemesis Restreamer installer"
git branch -M main
git remote add origin https://github.com/YOUR-USERNAME/restreamer-installer.git
git push -u origin main
```

### 2. Update URLs in Documentation

Replace `[YOUR-USERNAME]` with your GitHub username in these files:

**README.md:**
- Line 10: One-line installation command
- Multiple locations throughout the file

**QUICKSTART.md:**
- Line 13: One-line installation command
- Line 174: Uninstall command

**diagnose.sh:**
- Line 5: Usage comment

**Update these URLs:**
```bash
cd restreamer-installer

# macOS/Linux:
sed -i '' 's/\[YOUR-USERNAME\]/rainmanjam/g' README.md QUICKSTART.md diagnose.sh

# Or manually edit the files and replace [YOUR-USERNAME] with your actual username
```

### 3. Make Scripts Executable

Ensure scripts have the correct permissions:

```bash
chmod +x install.sh
chmod +x uninstall.sh
chmod +x diagnose.sh
```

### 4. Test the Installation

Before publishing, test the installer on a clean Linux VM:

```bash
# On a test Ubuntu/Debian VM:
curl -fsSL https://raw.githubusercontent.com/YOUR-USERNAME/restreamer-installer/main/install.sh | sudo bash

# Or download and run locally:
wget https://raw.githubusercontent.com/YOUR-USERNAME/restreamer-installer/main/install.sh
chmod +x install.sh
sudo bash install.sh
```

### 5. Create a Release (Optional)

Tag a release for version tracking:

```bash
git tag -a v1.0.0 -m "Initial release: Restreamer installer v1.0.0"
git push origin v1.0.0
```

## Repository Structure

```
restreamer-installer/
├── .github/
│   └── workflows/
│       └── test-install.yml     # CI/CD testing workflow
├── install.sh                    # Main installer script
├── uninstall.sh                  # Uninstaller script
├── diagnose.sh                   # Diagnostic tool
├── README.md                     # Main documentation
├── QUICKSTART.md                 # Quick start guide
├── SETUP.md                      # This file
└── LICENSE                       # GPL-2.0 license (copy from main repo)
```

## Adding a License

Copy the LICENSE file from the main obs-polyemesis repository:

```bash
cp ../LICENSE ./LICENSE
git add LICENSE
git commit -m "Add GPL-2.0 license"
git push
```

## Repository Settings

### Recommended Settings

1. **Description**: "One-line installer for datarhei Restreamer - automated Docker deployment with HTTPS, backups, and monitoring"

2. **Topics**: Add these topics for discoverability:
   - restreamer
   - datarhei
   - obs
   - streaming
   - docker
   - installation
   - linux
   - automation

3. **Website**: Link to the main project
   - `https://github.com/rainmanjam/obs-polyemesis`

### Enable GitHub Actions

Make sure GitHub Actions is enabled in repository settings to run the test workflow.

## Updating the Main Repository

Update the main obs-polyemesis repository to reference this installer:

### In obs-polyemesis README.md

Add a section about server installation:

```markdown
## Server Installation

Need to set up a Restreamer server? Use our one-line installer:

```bash
curl -fsSL https://raw.githubusercontent.com/rainmanjam/restreamer-installer/main/install.sh | sudo bash
```

For more information, see the [Restreamer Installer repository](https://github.com/rainmanjam/restreamer-installer).
```

### In obs-polyemesis DEPLOYMENT_GUIDE.md

Update the Docker deployment section to reference the new installer:

```markdown
## Quick Restreamer Server Setup

For automated installation, use the Polyemesis Restreamer Installer:

https://github.com/rainmanjam/restreamer-installer

One-line command:
```bash
curl -fsSL https://raw.githubusercontent.com/rainmanjam/restreamer-installer/main/install.sh | sudo bash
```
```

## Testing Checklist

Before announcing the release, verify:

- [ ] All URLs in documentation point to correct repository
- [ ] Scripts have executable permissions
- [ ] Installation works on Ubuntu 20.04
- [ ] Installation works on Ubuntu 22.04
- [ ] Installation works on Ubuntu 24.04
- [ ] Installation works on Debian 11/12
- [ ] HTTP mode works correctly
- [ ] HTTPS mode works correctly (test with a domain)
- [ ] Firewall rules are applied correctly
- [ ] Backup script creates backups
- [ ] Restore script restores from backup
- [ ] Management scripts work
- [ ] Diagnostic script identifies issues
- [ ] Uninstall script removes everything cleanly
- [ ] OBS Polyemesis plugin can connect to installed Restreamer

## Promotion

Once tested and ready, promote the installer:

### In obs-polyemesis Repository

1. Update main README.md with installer link
2. Add to DEPLOYMENT_GUIDE.md
3. Mention in release notes
4. Add to documentation

### Social/Community

1. Create a demo video showing the one-line installation
2. Post in OBS forums/subreddit (if appropriate)
3. Update any existing documentation or tutorials
4. Consider writing a blog post about the installer

## Maintenance

### Updating the Installer

When updating the installer:

1. Make changes in a feature branch
2. Test thoroughly on multiple distributions
3. Update version number if using semantic versioning
4. Create a pull request
5. Merge and tag a new release

### Monitoring Issues

Watch for:
- Installation failures on specific distributions
- Docker compatibility issues
- Firewall configuration problems
- SSL certificate issues
- Feature requests from users

## Support

Direct users to:
- **Installer issues**: https://github.com/YOUR-USERNAME/restreamer-installer/issues
- **Plugin issues**: https://github.com/rainmanjam/obs-polyemesis/issues
- **Restreamer issues**: https://docs.datarhei.com/restreamer/

## Future Enhancements

Potential improvements:

- [ ] Add support for more Linux distributions
- [ ] Create automated tests for all major distros
- [ ] Add option to install specific Restreamer versions
- [ ] Add support for custom ports
- [ ] Add option to configure behind existing reverse proxy
- [ ] Add support for IPv6
- [ ] Create Ansible playbook version
- [ ] Create Terraform module
- [ ] Add support for Docker Compose deployment
- [ ] Add support for Kubernetes deployment

---

**Ready to publish!** Follow the steps above to create your separate repository and make the installer available to the community.
