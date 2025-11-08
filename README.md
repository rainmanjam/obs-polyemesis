# OBS Polyemesis - Restreamer Control Plugin

[![License](https://img.shields.io/badge/license-GPL--2.0-blue.svg)](LICENSE)
[![OBS Studio](https://img.shields.io/badge/OBS%20Studio-28%2B-green.svg)](https://obsproject.com/)
[![CI Pipeline](https://github.com/rainmanjam/obs-polyemesis/actions/workflows/ci.yaml/badge.svg)](https://github.com/rainmanjam/obs-polyemesis/actions/workflows/ci.yaml)
[![Security](https://github.com/rainmanjam/obs-polyemesis/actions/workflows/security.yaml/badge.svg)](https://github.com/rainmanjam/obs-polyemesis/actions/workflows/security.yaml)
[![CodeQL](https://github.com/rainmanjam/obs-polyemesis/actions/workflows/security.yaml/badge.svg?event=schedule)](https://github.com/rainmanjam/obs-polyemesis/security/code-scanning)

A comprehensive OBS Studio plugin for controlling and monitoring [datarhei Restreamer](https://github.com/datarhei/restreamer) with advanced multistreaming capabilities including orientation-aware routing.

```
┌─────────────────┐      ┌──────────────┐      ┌─────────────────┐
│   OBS Studio    │─────►│  Restreamer  │─────►│  YouTube        │
│                 │      │   (API)      │      │  Twitch         │
│  Polyemesis     │      │              │      │  TikTok         │
│  Plugin ⚡      │◄─────│  Monitoring  │      │  Instagram      │
└─────────────────┘      └──────────────┘      └─────────────────┘
```

## ✨ Key Features

- 🎮 **Complete Restreamer Control**: Manage processes, monitor stats, view logs
- 📺 **Multiple Plugin Types**: Source, Output, and Dock UI
- 🌐 **Advanced Multistreaming**: Stream to multiple platforms simultaneously
- 📱 **Orientation-Aware**: Automatically route horizontal/vertical streams correctly
- 🎯 **Service-Specific Routing**: Pre-configured for Twitch, YouTube, TikTok, Instagram, and more
- 📊 **Real-time Monitoring**: CPU, memory, uptime, and session tracking
- ⚡ **Smart Transcoding**: Automatic video conversion between orientations

## 🚀 Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/rainmanjam/obs-polyemesis.git
cd obs-polyemesis

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Install
cmake --install build
```

### First Use

1. Open OBS Studio
2. Go to View → Docks → Restreamer Control
3. Configure your restreamer connection (host, port)
4. Click "Test Connection"
5. Start controlling your restreamer processes!

## 📖 Documentation

- **[User Guide](USER_GUIDE.md)** - Installation, setup, and usage instructions
- **[Building](BUILDING.md)** - Developer build instructions for all platforms
- **[Contributing](CONTRIBUTING.md)** - Development workflow and contribution guidelines
- **[CI/CD Workflow](docs/CICD_WORKFLOW.md)** - Pipeline architecture and automation
- **[Plugin Documentation](PLUGIN_DOCUMENTATION.md)** - Detailed feature descriptions and API reference

## 🎯 Use Cases

### Portrait Mobile Streaming
Stream vertical 9:16 content to TikTok, Instagram, while automatically converting to horizontal for Twitch and YouTube.

### Multi-Platform Gaming
Stream landscape gameplay to Twitch and YouTube in native 16:9, with automatic cropping for vertical platforms.

### Universal Broadcasting
One OBS setup, multiple platforms, correct orientations - automatically.

## 🛠️ Requirements

- **OBS Studio**: 28.0 or later
- **datarhei Restreamer**: Running instance (local or remote)
- **Dependencies**:
  - libcurl
  - jansson
  - Qt6

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details on:
- Development workflow (GitHub Flow)
- Code quality standards
- Testing requirements
- Pull request process

## 📝 License

GNU General Public License v2.0 or later - see [LICENSE](LICENSE) for details.

## 🙏 Credits

- [OBS Studio](https://obsproject.com/)
- [datarhei Restreamer](https://github.com/datarhei/restreamer)
- [OBS Plugin Template](https://github.com/obsproject/obs-plugintemplate)

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/rainmanjam/obs-polyemesis/issues)
- **Restreamer Docs**: [docs.datarhei.com/restreamer](https://docs.datarhei.com/restreamer/)

---

**Built with ❤️ for the streaming community**
