# Restreamer Installer - Maintenance Guide

This document explains the restreamer-installer integration in the obs-polyemesis repository and how to maintain it.

## Overview

The restreamer-installer is **integrated into the main obs-polyemesis repository** as a subdirectory, not a separate repository. This makes it easier to maintain and ensures version alignment with the OBS plugin.

## Repository Structure

```
obs-polyemesis/
├── restreamer-installer/          # Installer directory
│   ├── install.sh                 # Main installer script
│   ├── uninstall.sh              # Uninstaller script
│   ├── diagnose.sh               # Diagnostic tool
│   ├── test-local.sh             # Local testing script
│   ├── README.md                 # Main documentation
│   ├── QUICKSTART.md             # Quick start guide
│   ├── SETUP.md                  # This file
│   └── .gitignore                # Git ignore patterns
├── src/                          # OBS plugin source code
├── .github/workflows/            # GitHub Actions (shared)
└── ...                           # Other plugin files
```

## Installation URL

When merged to main, the installer is available at:

```bash
curl -fsSL https://raw.githubusercontent.com/rainmanjam/obs-polyemesis/main/restreamer-installer/install.sh | sudo bash
```

## Maintenance Tasks

### Updating the Installer

1. **Create a feature branch**
   ```bash
   git checkout -b feature/update-installer
   ```

2. **Make your changes** to files in `restreamer-installer/`

3. **Test locally** before pushing
   ```bash
   cd restreamer-installer
   ./test-local.sh
   ```

4. **Test syntax**
   ```bash
   bash -n install.sh
   bash -n uninstall.sh
   bash -n diagnose.sh
   shellcheck --severity=warning --exclude=SC2155 *.sh
   ```

5. **Commit and push**
   ```bash
   git add restreamer-installer/
   git commit -m "Update restreamer installer: [description]"
   git push -u origin feature/update-installer
   ```

6. **Create Pull Request** and merge to main

### Testing Changes

#### Local Testing (macOS/Linux)
```bash
cd restreamer-installer
./test-local.sh
```

This runs 41 automated tests including:
- Bash syntax validation
- ShellCheck analysis
- Required function checks
- State variable initialization
- Documentation completeness

#### Live Testing (Linux VM Required)
Test on actual Linux distributions before merging:

**Ubuntu/Debian:**
```bash
# Use your feature branch
curl -fsSL https://raw.githubusercontent.com/rainmanjam/obs-polyemesis/feature/your-branch/restreamer-installer/install.sh | sudo bash
```

**Recommended test environments:**
- Ubuntu 22.04 LTS
- Ubuntu 24.04 LTS
- Debian 12
- CentOS Stream 9

### Version Management

The installer doesn't have separate versioning - it follows the obs-polyemesis release cycle.

**When releasing a new version of OBS Polyemesis:**
1. Ensure installer is tested and working
2. Update main README.md to mention installer if needed
3. Include installer updates in release notes
4. Tag the release (applies to entire repository)

## Key Features to Maintain

### Critical Features
- ✅ SSH lockout protection
- ✅ DNS verification before SSL
- ✅ Smart rollback system
- ✅ Cross-distribution support
- ✅ Firewall auto-configuration

### When Adding New Features

1. **Update install.sh** with new functionality
2. **Update README.md** with feature documentation
3. **Update QUICKSTART.md** if it affects user workflow
4. **Add tests** to test-local.sh if applicable
5. **Test on multiple distributions**
6. **Update this SETUP.md** if maintenance changes

## Monitoring and Support

### Issue Tracking

Installer issues are tracked in the main obs-polyemesis repository:
- **URL**: https://github.com/rainmanjam/obs-polyemesis/issues
- **Label**: Use `installer` label for restreamer-installer issues

### Common Issues to Watch For

- Installation failures on specific distributions
- Docker compatibility issues
- Firewall configuration problems
- SSL certificate issues
- DNS verification false positives
- SSH port detection failures

## Testing Checklist

Before merging installer changes to main:

**Basic Validation:**
- [ ] `bash -n` syntax check passes for all scripts
- [ ] `./test-local.sh` passes (41/41 tests)
- [ ] ShellCheck passes (or warnings are acceptable)
- [ ] All URLs point to correct paths

**Functionality Testing:**
- [ ] HTTP mode installation works
- [ ] HTTPS mode installation works (with domain)
- [ ] DNS verification works correctly
- [ ] SSH protection prevents lockouts
- [ ] Firewall rules are applied correctly
- [ ] Rollback system works on cancellation
- [ ] Backup/restore scripts function
- [ ] Monitoring tools install correctly
- [ ] Uninstall removes everything cleanly

**Distribution Testing:**
- [ ] Ubuntu 22.04 LTS
- [ ] Ubuntu 24.04 LTS
- [ ] Debian 12
- [ ] CentOS Stream 9 (if possible)

**Integration Testing:**
- [ ] OBS Polyemesis plugin can connect to installed Restreamer
- [ ] Streaming works end-to-end

## Documentation Standards

### File Purposes

- **README.md**: Complete reference documentation
- **QUICKSTART.md**: 5-minute getting started guide
- **SETUP.md**: This file - maintenance guide

### When to Update Documentation

**Update README.md when:**
- Adding new features
- Changing installation flow
- Adding new requirements
- Updating supported distributions
- Changing default ports or settings

**Update QUICKSTART.md when:**
- Installation steps change
- User prompts change
- Quick start flow is affected

**Update SETUP.md (this file) when:**
- Maintenance procedures change
- Testing requirements change
- Repository structure changes

## Future Enhancements

Planned improvements:

- [ ] Support for more Linux distributions (OpenSUSE, Alpine)
- [ ] Automated tests on actual VMs (GitHub Actions with Act)
- [ ] Custom port configuration option
- [ ] Support for existing reverse proxy setups
- [ ] IPv6 support
- [ ] Ansible playbook version
- [ ] Docker Compose alternative
- [ ] Non-interactive mode for automation

## Contributing

If you're contributing to the installer:

1. Read this SETUP.md guide
2. Test your changes thoroughly
3. Update documentation
4. Run `./test-local.sh` before committing
5. Create a pull request with clear description

## Getting Help

- **Plugin issues**: Main obs-polyemesis repository
- **Installer issues**: Same repository, use `installer` label
- **Restreamer issues**: https://docs.datarhei.com/restreamer/
- **Docker issues**: https://docs.docker.com/

---

**Last Updated**: November 2024
**Installer Status**: Integrated into obs-polyemesis v0.9.0+
