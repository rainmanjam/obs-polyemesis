# Local CI/CD Testing with ACT

This guide shows how to test GitHub Actions workflows locally using [ACT](https://github.com/nektos/act).

## Prerequisites

- Docker Desktop must be running
- ACT installed (`brew install act` on macOS)
- At least 20GB of free disk space (for container images)

## Quick Start

```bash
# Test all jobs (will take a while on first run)
act push

# Test specific platform
act -j ubuntu-build
act -j windows-build
act -j macos-build  # May not work perfectly due to Xcode requirements

# Test pull request workflow
act pull_request

# Dry run (show what would execute)
act -n

# List all workflows
act -l
```

## Useful Commands

### Run Specific Job

```bash
# Run just the Ubuntu build
act push -j ubuntu-build

# Run with specific event data
act push --eventpath .github/event.json
```

### Debug Mode

```bash
# Verbose output
act -v

# Extra verbose (show all Docker commands)
act -vv

# Keep containers after run (for debugging)
act --reuse
```

### Environment Variables

```bash
# Pass secrets
act -s GITHUB_TOKEN=your_token_here

# Pass environment variables
act --env CUSTOM_VAR=value
```

### Working with Artifacts

```bash
# Artifacts are stored in /tmp/artifacts by default (see .actrc)
ls /tmp/artifacts

# Specify custom artifact directory
act --artifact-server-path ./artifacts
```

## Platform-Specific Notes

### Linux (Ubuntu) Builds

- ✅ Works perfectly with ACT
- Uses `catthehacker/ubuntu:act-latest` container
- First run downloads ~2GB of images

```bash
# Fast Linux testing
act -j ubuntu-build
```

### Windows Builds

- ✅ **NEW**: Native Windows testing with remote execution
- Use a Windows 11 machine with SSH for authentic Windows builds
- Control from Mac using helper scripts
- See [Windows Testing Guide](./WINDOWS_TESTING.md) for setup

```bash
# Quick Windows testing (after setup)
./scripts/windows-act.sh -j build-windows
./scripts/sync-and-build-windows.sh
./scripts/windows-package.sh --fetch
```

**Why Remote Windows Testing?**
- Native Visual Studio compilation
- Real NSIS installer creation
- Authentic Windows environment
- No container limitations

### macOS Builds

- ⚠️ Very limited support
- Requires macOS-in-Docker (experimental)
- Xcode licensing issues
- Recommended: Use GitHub Actions for macOS builds

## Troubleshooting

### Error: "Cannot connect to Docker daemon"

**Solution**: Start Docker Desktop

```bash
open -a Docker
```

### Error: "No space left on device"

**Solution**: Clean up Docker images

```bash
docker system prune -a
```

### Error: "Container failed to start"

**Solution**: Update container images

```bash
docker pull catthehacker/ubuntu:act-latest
```

### Slow Performance

**Solution**: Increase Docker resources

1. Open Docker Desktop
2. Settings → Resources
3. Increase CPUs and Memory
4. Restart Docker

## Configuration Files

### .actrc

Located in project root. Contains default ACT settings:

```
-P ubuntu-latest=catthehacker/ubuntu:act-latest
--artifact-server-path /tmp/artifacts
-v
```

### Custom Event Data

Create `.github/event.json` for custom event payloads:

```json
{
  "push": {
    "ref": "refs/heads/main"
  }
}
```

## Best Practices

1. **Test locally before pushing** - Catch errors early
2. **Use specific jobs** - Faster than running all jobs
3. **Clean up regularly** - ACT can use lots of disk space
4. **Focus on Linux** - Most reliable platform for local testing
5. **Use GitHub Actions for full testing** - macOS/Windows are better tested in CI

## Comparison: ACT vs GitHub Actions

| Feature | ACT (Local) | GitHub Actions |
|---------|-------------|----------------|
| Speed | Fast (after first run) | Depends on queue |
| Cost | Free | Free (with limits) |
| Linux | ✅ Excellent | ✅ Excellent |
| macOS | ⚠️ Limited | ✅ Excellent |
| Windows | ⚠️ Limited | ✅ Excellent |
| Accuracy | ~90% | 100% |
| Setup | Requires Docker | None |

## Example Workflows

### Pre-commit Testing

```bash
#!/bin/bash
# test.sh - Run before committing

echo "Testing build locally..."
act -j ubuntu-build

if [ $? -eq 0 ]; then
    echo "✅ Build passed! Safe to commit."
else
    echo "❌ Build failed! Fix errors before committing."
    exit 1
fi
```

### Continuous Local Testing

```bash
# Watch for changes and auto-test
while inotifywait -e modify src/**/*.c; do
    act -j ubuntu-build
done
```

## Resources

- ACT Documentation: https://github.com/nektos/act
- Container Images: https://github.com/catthehacker/docker_images
- Troubleshooting: https://github.com/nektos/act/issues

## Support

For issues specific to ACT testing:
1. Check Docker is running
2. Update ACT: `brew upgrade act`
3. Update containers: `docker pull <image>`
4. Review logs with `-v` flag

For workflow issues, compare ACT output with GitHub Actions logs.
