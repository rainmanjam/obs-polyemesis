# OBS Polyemesis v1.0.0 - Release Notes

**Release Date**: November 8, 2025
**Version**: 1.0.0
**Codename**: "Polymorphic Stream"

---

## 🎉 Welcome to OBS Polyemesis v1.0!

We're excited to announce the first stable release of OBS Polyemesis, a comprehensive plugin for controlling datarhei Restreamer with advanced multistreaming capabilities directly from OBS Studio.

---

## 🌟 What is OBS Polyemesis?

OBS Polyemesis transforms how you interact with datarhei Restreamer by providing:

- **Full Restreamer Control** - Start, stop, and monitor processes without leaving OBS
- **Smart Multistreaming** - Automatically route content to the right platforms in the correct orientation
- **Real-time Monitoring** - Track CPU, memory, uptime, and active sessions
- **Professional UI** - Clean Qt-based dock panel that feels native to OBS

---

## ✨ Key Features

### 1. Comprehensive Restreamer Integration

- Connect to local or remote Restreamer instances
- Full API v3 support (datarhei-core v16+)
- HTTP Basic Authentication
- Secure HTTPS/TLS connections
- Real-time process status updates

### 2. Intelligent Multistreaming

**Automatic Orientation Detection:**
- Detects landscape (16:9), portrait (9:16), and square (1:1) video
- Routes streams to orientation-appropriate platforms
- Auto-generates FFmpeg filter chains for format conversion

**Platform Support:**
- **Landscape Preferred**: Twitch, Kick
- **Vertical Preferred**: TikTok, Instagram
- **All Orientations**: YouTube, Facebook, X/Twitter
- **Custom RTMP**: Any streaming service

### 3. Multiple Plugin Types

- **Source Plugin**: Display Restreamer streams in your scenes
- **Output Plugin**: Stream directly to Restreamer
- **Dock UI**: Full-featured control panel

### 4. Production-Ready Quality

- **13 test cases** - 100% passing on all platforms
- **Cross-platform** - macOS, Windows, Linux
- **Memory safe** - No leaks detected with AddressSanitizer
- **Performance tested** - Benchmarked for efficiency
- **Professionally packaged** - Native installers for each platform

---

## 📦 Installation

### macOS
```bash
# Download the .pkg installer
# Double-click to install, or:
sudo installer -pkg obs-polyemesis-1.0.0-macos.pkg -target /
```

### Windows
```powershell
# Download and run the .exe installer
obs-polyemesis-1.0.0-windows-x64.exe
```

### Linux (Debian/Ubuntu)
```bash
# Download and install the .deb package
sudo dpkg -i obs-polyemesis_1.0.0_amd64.deb
sudo apt-get install -f  # Install dependencies
```

### Linux (Fedora/RHEL)
```bash
# Download and install the .rpm package
sudo dnf install obs-polyemesis-1.0.0-1.x86_64.rpm
```

---

## 🚀 Quick Start

1. **Install the plugin** using your platform's installer
2. **Launch OBS Studio** (version 28.0 or later required)
3. **Open the dock**: View → Docks → Restreamer Control
4. **Configure connection**:
   - Host: `localhost` (or your Restreamer server)
   - Port: `8080` (default Restreamer port)
   - Click "Test Connection"
5. **Start managing** your streams!

---

## 🎯 Use Cases

### Content Creators
- Stream to multiple platforms simultaneously
- Automatic format optimization per platform
- Monitor all streams from one interface

### Live Event Producers
- Professional multi-destination streaming
- Real-time monitoring and control
- Instant stream adjustments without leaving OBS

### Mobile Streamers
- Perfect for portrait/vertical content
- Auto-route to TikTok, Instagram (vertical)
- Also stream to YouTube, Twitch (converted)

---

## 📊 Technical Specifications

### System Requirements

**Minimum:**
- OBS Studio 28.0+
- macOS 11.0 (Big Sur) / Windows 10 / Ubuntu 24.04
- 4GB RAM
- Network connection to Restreamer instance

**Recommended:**
- OBS Studio 30.0+
- macOS 14.0 (Sonoma) / Windows 11 / Ubuntu 24.04
- 8GB RAM
- Dedicated network connection

### Dependencies

- libcurl (HTTP client)
- jansson (JSON parsing)
- Qt6 Core and Widgets

### Performance

Based on benchmark testing:
- **API Calls**: <0.01ms per operation
- **Orientation Detection**: 250,000+ ops/sec
- **Memory Footprint**: <10MB
- **CPU Usage**: <1% idle, <5% active

---

## 🔧 Configuration

### Global Connection Settings

Set once, used everywhere:
- Restreamer host and port
- HTTPS enabled/disabled
- Authentication credentials

### Per-Source Settings

Override global settings for specific sources:
- Custom Restreamer URL
- Different authentication
- Source-specific configurations

### Multistream Configuration

- Enable/disable auto-orientation detection
- Manual orientation override
- Per-destination enable/disable
- Stream key management

---

## 📝 What's New in v1.0

### Core Features (NEW)
- ✨ Complete Restreamer API integration
- ✨ Three plugin types (Source, Output, Dock)
- ✨ Qt6-based control panel
- ✨ Real-time process monitoring
- ✨ Session tracking and statistics

### Multistreaming (NEW)
- ✨ Automatic orientation detection
- ✨ Intelligent platform routing
- ✨ Support for 7+ major platforms
- ✨ Custom RTMP destinations
- ✨ Auto-generated FFmpeg filters

### Quality & Testing (NEW)
- ✨ 13 comprehensive test cases
- ✨ Cross-platform CI/CD
- ✨ Performance benchmarks
- ✨ Memory leak detection
- ✨ Code coverage reporting

### Distribution (NEW)
- ✨ Native macOS .pkg installer
- ✨ Windows .exe installer (NSIS)
- ✨ Linux .deb packages
- ✨ Linux .rpm packages

---

## 🐛 Known Issues

1. **API Compatibility**: Currently tested with datarhei-core v16.16.0. Other versions may have different API structures.

2. **Code Signing**: macOS and Windows packages are currently unsigned. You may need to allow installation in system settings.

3. **UI Tests**: Automated UI testing framework is in development. Current testing is manual.

4. **Real-World Testing**: Integration with production Restreamer instances is pending community validation.

---

## 🔮 Roadmap

### v1.1 (Next Release)
- Automated UI tests
- Enhanced error recovery
- Performance optimizations
- Additional platform integrations

### v1.2 (Future)
- WebSocket support for real-time updates
- Process template system
- Batch operations
- Advanced monitoring dashboard

### v2.0 (Long-term)
- Plugin API for extensibility
- Custom filter framework
- Multi-Restreamer support
- Cloud integration options

---

## 🤝 Contributing

We welcome contributions! Here's how you can help:

### Report Issues
- Use GitHub Issues for bug reports
- Provide detailed reproduction steps
- Include OBS and plugin versions
- Attach relevant logs

### Submit Pull Requests
- Follow the code style guide (CODE_STYLE.md)
- Include tests for new features
- Update documentation as needed
- Ensure all CI/CD checks pass

### Share Feedback
- Star the repository if you find it useful
- Share your use cases and success stories
- Suggest features and improvements
- Help others in discussions

---

## 📚 Documentation

- **README**: Project overview and quick start
- **PLUGIN_DOCUMENTATION**: Complete feature documentation
- **BUILDING**: Platform-specific build instructions
- **DEPLOYMENT_GUIDE**: Docker setup and deployment
- **CHANGELOG**: Detailed change history
- **CODE_STYLE**: Formatting and style guidelines

---

## 🙏 Acknowledgments

- **OBS Studio Team** - For the amazing streaming software
- **datarhei** - For the excellent Restreamer project
- **Contributors** - Everyone who tested and provided feedback
- **Community** - For your support and enthusiasm

---

## 📞 Support

### Getting Help
- **Documentation**: Check PLUGIN_DOCUMENTATION.md
- **Issues**: https://github.com/rainmanjam/obs-polyemesis/issues
- **Discussions**: https://github.com/rainmanjam/obs-polyemesis/discussions

### Commercial Support
For professional support, integration help, or custom development:
- Contact via GitHub Issues (tag: commercial-support)

---

## 📄 License

GNU General Public License v2.0 or later

This plugin is free and open-source software. You can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation.

---

## 🎊 Thank You!

Thank you for choosing OBS Polyemesis! We're excited to see what you create with it.

Happy streaming! 🎥🚀

---

**Download**: [GitHub Releases](https://github.com/rainmanjam/obs-polyemesis/releases/tag/v1.0.0)
**Source Code**: [GitHub Repository](https://github.com/rainmanjam/obs-polyemesis)
**Website**: https://github.com/rainmanjam/obs-polyemesis

---

*OBS Polyemesis v1.0.0 - Empowering creators with intelligent multistreaming*
