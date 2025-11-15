"""
Platform detection and helper utilities for cross-platform testing.

This module provides platform-agnostic helpers for Windows, macOS, and Linux testing.
"""

import platform
import os
import sys
from pathlib import Path
from typing import Optional, Dict, List
import json


class PlatformInfo:
    """Detect and provide information about the current platform."""

    def __init__(self):
        self.system = platform.system()
        self.is_windows = self.system == "Windows"
        self.is_macos = self.system == "Darwin"
        self.is_linux = self.system == "Linux"
        self.machine = platform.machine()
        self.python_version = platform.python_version()

    def __str__(self) -> str:
        return f"{self.system} ({self.machine})"

    @property
    def obs_plugin_dir(self) -> Path:
        """Get the OBS plugin directory for the current platform."""
        if self.is_windows:
            appdata = os.getenv('APPDATA', '')
            return Path(appdata) / "obs-studio" / "plugins"
        elif self.is_macos:
            return Path.home() / "Library" / "Application Support" / "obs-studio" / "plugins"
        elif self.is_linux:
            return Path.home() / ".config" / "obs-studio" / "plugins"
        else:
            raise NotImplementedError(f"Unsupported platform: {self.system}")

    @property
    def obs_executable_name(self) -> str:
        """Get the OBS executable name for the current platform."""
        if self.is_windows:
            return "obs64.exe"
        elif self.is_macos:
            return "OBS"
        elif self.is_linux:
            return "obs"
        else:
            raise NotImplementedError(f"Unsupported platform: {self.system}")

    @property
    def docker_socket(self) -> str:
        """Get the Docker socket path for the current platform."""
        if self.is_windows:
            return "npipe:////./pipe/docker_engine"
        else:
            return "unix:///var/run/docker.sock"


def get_platform_info() -> PlatformInfo:
    """Get platform information."""
    return PlatformInfo()


def get_config_dir() -> Path:
    """Get the tests configuration directory."""
    return Path(__file__).parent.parent / "fixtures"


def load_test_config() -> Dict:
    """Load the test configuration from JSON."""
    config_path = get_config_dir() / "restreamer_config.json"
    with open(config_path, 'r') as f:
        return json.load(f)


def get_plugin_install_path(plugin_name: str = "obs-polyemesis") -> Path:
    """
    Get the full path where the plugin should be installed.

    Args:
        plugin_name: Name of the plugin (default: obs-polyemesis)

    Returns:
        Path to the plugin installation directory
    """
    platform_info = get_platform_info()
    return platform_info.obs_plugin_dir / plugin_name


def get_plugin_binary_name(plugin_name: str = "obs-polyemesis") -> str:
    """
    Get the plugin binary name for the current platform.

    Args:
        plugin_name: Name of the plugin (default: obs-polyemesis)

    Returns:
        Binary filename with platform-specific extension
    """
    platform_info = get_platform_info()

    if platform_info.is_windows:
        return f"{plugin_name}.dll"
    elif platform_info.is_macos:
        return f"{plugin_name}.so"  # or .dylib depending on OBS
    elif platform_info.is_linux:
        return f"{plugin_name}.so"
    else:
        raise NotImplementedError(f"Unsupported platform: {platform_info.system}")


def find_obs_executable() -> Optional[Path]:
    """
    Find the OBS executable on the current system.

    Returns:
        Path to OBS executable if found, None otherwise
    """
    platform_info = get_platform_info()
    exe_name = platform_info.obs_executable_name

    # Common installation paths
    if platform_info.is_windows:
        search_paths = [
            Path("C:/Program Files/obs-studio/bin/64bit") / exe_name,
            Path("C:/Program Files (x86)/obs-studio/bin/64bit") / exe_name,
            Path(os.getenv('ProgramFiles', 'C:/Program Files')) / "obs-studio" / "bin" / "64bit" / exe_name,
        ]
    elif platform_info.is_macos:
        search_paths = [
            Path("/Applications/OBS.app/Contents/MacOS") / exe_name,
            Path.home() / "Applications" / "OBS.app" / "Contents" / "MacOS" / exe_name,
        ]
    elif platform_info.is_linux:
        search_paths = [
            Path("/usr/bin") / exe_name,
            Path("/usr/local/bin") / exe_name,
            Path.home() / ".local" / "bin" / exe_name,
        ]
    else:
        return None

    for path in search_paths:
        if path.exists():
            return path

    # Try PATH
    import shutil
    path_result = shutil.which(exe_name)
    if path_result:
        return Path(path_result)

    return None


def get_environment_variables() -> Dict[str, str]:
    """
    Get platform-specific environment variables for testing.

    Returns:
        Dictionary of environment variables
    """
    platform_info = get_platform_info()
    config = load_test_config()

    env_vars = {
        "RESTREAMER_HOST": "localhost",
        "RESTREAMER_PORT": "8080",
        "RESTREAMER_USER": config["credentials"]["username"],
        "RESTREAMER_PASS": config["credentials"]["password"],
        "RESTREAMER_BASE_URL": config["api"]["base_url"],
    }

    # Platform-specific additions
    if platform_info.is_windows:
        env_vars["PATH"] = os.environ.get("PATH", "")

    return env_vars


def is_docker_available() -> bool:
    """
    Check if Docker is available and running.

    Returns:
        True if Docker is available, False otherwise
    """
    import shutil
    import subprocess

    if not shutil.which("docker"):
        return False

    try:
        result = subprocess.run(
            ["docker", "info"],
            capture_output=True,
            timeout=5
        )
        return result.returncode == 0
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return False


def get_docker_compose_command() -> List[str]:
    """
    Get the appropriate docker-compose command for the platform.

    Returns:
        List of command components
    """
    import shutil

    # Try docker compose (v2, integrated)
    if shutil.which("docker"):
        try:
            import subprocess
            result = subprocess.run(
                ["docker", "compose", "version"],
                capture_output=True,
                timeout=5
            )
            if result.returncode == 0:
                return ["docker", "compose"]
        except:
            pass

    # Try docker-compose (v1, standalone)
    if shutil.which("docker-compose"):
        return ["docker-compose"]

    raise RuntimeError("Neither 'docker compose' nor 'docker-compose' found")


# Convenience singleton
platform_info = get_platform_info()

__all__ = [
    "PlatformInfo",
    "get_platform_info",
    "get_config_dir",
    "load_test_config",
    "get_plugin_install_path",
    "get_plugin_binary_name",
    "find_obs_executable",
    "get_environment_variables",
    "is_docker_available",
    "get_docker_compose_command",
    "platform_info",
]
