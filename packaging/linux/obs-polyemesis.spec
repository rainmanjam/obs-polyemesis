# OBS Polyemesis RPM Spec File
# For Fedora, RHEL, CentOS, openSUSE, etc.
#
# Build command:
#   rpmbuild -bb obs-polyemesis.spec
#
# Or using a build script:
#   ./create-rpm.sh 1.0.0

%global commit_short %(echo %{version} | cut -c1-7)

Name:           obs-polyemesis
Version:        1.0.0
Release:        1%{?dist}
Summary:        datarhei Restreamer control plugin for OBS Studio

License:        GPL-2.0-or-later
URL:            https://github.com/rainmanjam/%{name}
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  libcurl-devel
BuildRequires:  jansson-devel
BuildRequires:  obs-studio-devel >= 28.0
BuildRequires:  qt6-qtbase-devel

Requires:       obs-studio >= 28.0
Requires:       libcurl
Requires:       jansson

%description
OBS Polyemesis is a comprehensive plugin for controlling and monitoring
datarhei Restreamer with advanced multistreaming capabilities including
orientation-aware routing.

Features:
 - Complete Restreamer control from within OBS
 - Multiple plugin types: Source, Output, and Dock UI
 - Advanced multistreaming to multiple platforms
 - Orientation-aware routing (horizontal/vertical)
 - Real-time monitoring of CPU, memory, uptime

%prep
%setup -q

%build
%cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_FRONTEND_API=ON \
    -DENABLE_QT=ON
%cmake_build

%install
%cmake_install

# Install locale files
mkdir -p %{buildroot}%{_datadir}/obs/obs-plugins/%{name}/locale
if [ -d data/locale ]; then
    cp -r data/locale/* %{buildroot}%{_datadir}/obs/obs-plugins/%{name}/locale/
fi

%files
%license LICENSE
%doc README.md
%{_libdir}/obs-plugins/lib%{name}.so
%{_datadir}/obs/obs-plugins/%{name}/

%post
echo "OBS Polyemesis plugin installed successfully!"
echo ""
echo "To use the plugin:"
echo "1. Launch OBS Studio"
echo "2. Go to View → Docks → Restreamer Control"
echo "3. Configure your restreamer connection"
echo ""
echo "For documentation, visit:"
echo "https://github.com/rainmanjam/%{name}"

%changelog
* Fri Nov 08 2025 rainmanjam <noreply@github.com> - 1.0.0-1
- Initial RPM release
- Full datarhei Restreamer integration
- Multi-platform multistreaming support
- Orientation-aware routing
- Qt6-based control dock
