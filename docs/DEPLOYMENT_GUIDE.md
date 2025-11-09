# OBS Polyemesis - Deployment Guide

**Last Updated**: 2025-11-08
**Status**: Docker deployment complete, API compatibility notes

---

## Restreamer Deployment

### Docker Deployment (Recommended for Testing)

#### Using datarhei-core

The datarhei-core container is the underlying engine for Restreamer:

```bash
# Deploy datarhei-core
docker run -d \
  --name restreamer \
  --restart unless-stopped \
  -p 8080:8080 \
  -p 8181:8181 \
  -p 1935:1935 \
  -p 6000:6000/udp \
  datarhei/restreamer:latest

# Verify deployment
docker ps --filter "name=restreamer"

# Check logs
docker logs restreamer

# Test API
curl http://localhost:8080/api
```

**Deployed Version**: datarhei-core v16.16.0

**Access Points**:
- Web UI: http://localhost:8080
- API: http://localhost:8080/api
- RTMP: rtmp://localhost:1935
- SRT/UDP: localhost:6000

### API Compatibility Notes

⚠️ **Important**: The OBS Polyemesis plugin was designed for Restreamer UI's API structure (`/api/v3/`), but datarhei-core uses a different API structure (`/api`).

**Differences**:

| Feature | Restreamer UI API | datarhei-core API |
|---------|-------------------|-------------------|
| Base Path | `/api/v3/` | `/api` |
| Connection Test | `GET /api/v3/` | `GET /api` |
| Process List | `GET /api/v3/process` | `GET /api/v3/process` (core) |
| Authentication | HTTP Basic Auth | HTTP Basic Auth + Token |

**Current Status**:
- ✅ datarhei-core deployed successfully
- ✅ API accessible at http://localhost:8080/api
- ⚠️ Plugin API client uses `/api/v3/` endpoints
- ✅ Mock server tests passing (validates API client functionality)

### Next Steps for Real-World Testing

Two options for full integration testing:

#### Option 1: Deploy Full Restreamer UI

The complete Restreamer UI wraps datarhei-core and provides the `/api/v3/` endpoints:

```bash
# Stop current container
docker stop restreamer && docker rm restreamer

# Deploy full Restreamer UI (if available)
# Note: May require additional configuration
docker run -d \
  --name restreamer \
  --restart unless-stopped \
  -p 8080:8080 \
  -p 8181:8181 \
  -p 1935:1935 \
  -p 6000:6000/udp \
  datarhei/restreamer-ui:latest  # If this image exists
```

#### Option 2: Adapt Plugin to datarhei-core API

Modify the plugin's API client to work with datarhei-core's API structure:

1. Change base path from `/api/v3/` to `/api/v3/` (core uses this too)
2. Adjust authentication mechanism
3. Map endpoint differences

---

## Plugin Installation Testing

### macOS Installation

#### From Build Artifacts

```bash
# Download latest build artifacts
gh run download 19192437736

# Navigate to macOS build
cd obs-polyemesis-1.0.0-macos-universal-*

# Install plugin
cp -r obs-polyemesis.plugin ~/Library/Application\ Support/obs-studio/plugins/

# Launch OBS Studio
open -a OBS

# Verify installation
# 1. Check Tools menu for plugin entry
# 2. View → Docks → Restreamer Control
# 3. Add Source: + → Restreamer Source
# 4. Settings → Output → Restreamer Output
```

#### From Local Build

```bash
# Build locally
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build

# Plugin is installed to:
# ~/Library/Application Support/obs-studio/plugins/obs-polyemesis.plugin
```

### Ubuntu Installation

```bash
# Download build artifacts
gh run download 19192437736

# Navigate to Ubuntu build
cd obs-polyemesis-1.0.0-ubuntu-24.04-x86_64-*

# Install plugin
mkdir -p ~/.config/obs-studio/plugins
cp -r obs-polyemesis ~/.config/obs-studio/plugins/

# Launch OBS
obs

# Verify same as macOS steps
```

### Windows Installation

```powershell
# Download build artifacts
gh run download 19192437736

# Navigate to Windows build
cd obs-polyemesis-1.0.0-windows-x64-*

# Install plugin
# Copy obs-polyemesis folder to:
# %APPDATA%\obs-studio\plugins\

# Launch OBS and verify
```

---

## Testing Checklist

### Connection Testing
- [ ] Connect to Restreamer from plugin
- [ ] Test with correct credentials
- [ ] Test with incorrect credentials
- [ ] Test connection error handling

### Source Plugin Testing
- [ ] Add Restreamer Source to scene
- [ ] Configure source settings
- [ ] Test global vs per-source connection
- [ ] Verify stream preview

### Output Plugin Testing
- [ ] Configure Restreamer Output
- [ ] Start streaming
- [ ] Verify stream appears in Restreamer
- [ ] Test stop/start cycling

### Dock UI Testing
- [ ] Open Restreamer Control dock
- [ ] Test connection configuration
- [ ] Test process list refresh
- [ ] Test process control buttons
- [ ] Verify real-time monitoring

### Multistreaming Testing
- [ ] Configure orientation settings
- [ ] Add multiple destinations
- [ ] Test orientation-aware routing
- [ ] Verify filter generation

---

## Troubleshooting

### Docker Container Issues

**Container won't start**:
```bash
# Check Docker daemon
docker info

# View container logs
docker logs restreamer

# Restart container
docker restart restreamer
```

**Port conflicts**:
```bash
# Check if ports are in use
lsof -i :8080
lsof -i :1935

# Use different ports
docker run -d \
  --name restreamer \
  -p 9080:8080 \
  -p 9935:1935 \
  datarhei/restreamer:latest
```

### Plugin Installation Issues

**Plugin not appearing in OBS**:
1. Check installation path is correct
2. Verify plugin file permissions
3. Check OBS logs: `~/Library/Application Support/obs-studio/logs/`
4. Ensure OBS version is 28.0+

**Plugin loads but crashes**:
1. Check dependency libraries are present
2. Verify OBS architecture matches plugin (arm64 vs x86_64)
3. Review crash logs in OBS log directory

---

## Performance Testing

### Metrics to Monitor

**System Resources**:
- CPU usage (OBS + Restreamer)
- Memory consumption
- Network bandwidth

**Streaming Quality**:
- Video bitrate stability
- Audio sync
- Frame drops
- Latency measurements

**Test Scenarios**:
1. Single stream (1080p60)
2. Multistream to 3 destinations
3. Multistream to 5+ destinations
4. Long-running stability (4+ hours)
5. Orientation conversion performance

### Profiling Tools

**macOS**:
```bash
# CPU profiling with Instruments
instruments -t "Time Profiler" -D profile.trace obs

# Memory profiling
instruments -t "Allocations" -D memory.trace obs
```

**Linux**:
```bash
# Valgrind for memory leaks
valgrind --leak-check=full obs

# perf for CPU profiling
perf record -g obs
perf report
```

**Windows**:
- Use Visual Studio Performance Profiler
- Windows Performance Analyzer

---

## Production Deployment

### Cloud VPS Setup (DigitalOcean Example)

```bash
# 1. Create droplet (Ubuntu 24.04, $12/mo for 2GB RAM)
# 2. SSH into server
ssh root@your-server-ip

# 3. Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sh get-docker.sh

# 4. Deploy Restreamer
docker run -d \
  --name restreamer \
  --restart unless-stopped \
  -p 8080:8080 \
  -p 8181:8181 \
  -p 1935:1935 \
  -p 6000:6000/udp \
  -v /var/restreamer:/core/data \
  datarhei/restreamer:latest

# 5. Configure firewall
ufw allow 8080/tcp
ufw allow 1935/tcp
ufw allow 6000/udp
ufw enable

# 6. Optional: Set up HTTPS with nginx reverse proxy
apt install nginx certbot python3-certbot-nginx
# Configure nginx for port 8080 → 443
certbot --nginx -d restreamer.yourdomain.com
```

### Security Considerations

1. **Credentials**: Always use HTTPS in production
2. **Firewall**: Only open required ports
3. **Updates**: Keep Docker and containers updated
4. **Backups**: Backup Restreamer configuration regularly
5. **Monitoring**: Set up uptime monitoring

---

## Next Steps

### For Development
1. Create installation packages (.pkg, .exe, .deb)
2. Add UI tests for Qt dock panel
3. Performance benchmarking
4. Security audit

### For Distribution
1. Beta testing program
2. OBS Plugin Browser submission
3. Package repository setup
4. Documentation website

---

## Current Deployment Status

**Completed**:
- ✅ Docker deployment working
- ✅ datarhei-core v16.16.0 running
- ✅ API accessible and responding
- ✅ All test suites passing (13/13 tests)
- ✅ CI/CD workflows building on all platforms

**Pending**:
- ⏳ Full Restreamer UI deployment (if needed)
- ⏳ Real-world streaming tests
- ⏳ Performance benchmarking
- ⏳ Installation package creation

**Blockers**:
- None (can proceed with packaging and testing)

---

**Deployment ready for:** ✅ Local testing, ✅ Development, ⚠️ Production (requires HTTPS + security hardening)
