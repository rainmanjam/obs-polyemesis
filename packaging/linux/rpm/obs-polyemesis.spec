Name:           obs-polyemesis
Version:        1.0.0
Release:        1%{?dist}
Summary:        datarhei Restreamer control plugin for OBS Studio

License:        GPLv2+
URL:            https://github.com/rainmanjam/obs-polyemesis
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.28
BuildRequires:  gcc-c++
BuildRequires:  obs-studio-devel
BuildRequires:  libcurl-devel
BuildRequires:  jansson-devel
BuildRequires:  qt6-qtbase-devel
BuildRequires:  pkgconfig

Requires:       obs-studio >= 28.0
Requires:       libcurl
Requires:       jansson
Requires:       qt6-qtbase

%description
OBS Polyemesis is a comprehensive plugin for controlling and monitoring
datarhei Restreamer instances with advanced multistreaming capabilities.

Features include:
- Full Restreamer process control and monitoring
- Real-time statistics and session tracking
- Advanced multistreaming with orientation detection
- Support for major platforms (Twitch, YouTube, TikTok, Instagram)
- Qt-based control panel with intuitive UI
- Multiple plugin types (Source, Output, Dock)

%prep
%autosetup

%build
%cmake -DENABLE_FRONTEND_API=ON -DENABLE_QT=ON
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md PLUGIN_DOCUMENTATION.md
%{_libdir}/obs-plugins/obs-polyemesis.so
%{_datadir}/obs/obs-plugins/obs-polyemesis/

%changelog
* Fri Nov 08 2025 rainmanjam <noreply@github.com> - 1.0.0-1
- Initial release
- Full Restreamer API integration
- Multistreaming with orientation support
- Cross-platform support (Linux, macOS, Windows)
