"""
OBS Studio helper utilities for cross-platform testing.

This module provides utilities for:
- Detecting OBS Studio installation
- Managing OBS plugin installation and verification
- Launching OBS with test parameters
- Collecting and analyzing OBS logs
"""

import os
import sys
import shutil
import subprocess
import time
from pathlib import Path
from typing import Optional, List, Dict, Tuple
import json

from .platform_helpers import (
    PlatformInfo,
    get_platform_info,
    get_plugin_install_path,
    get_plugin_binary_name,
    find_obs_executable
)


class OBSManager:
    """Manage OBS Studio for testing."""

    def __init__(self, plugin_name: str = "obs-polyemesis"):
        """
        Initialize OBS manager.

        Args:
            plugin_name: Name of the plugin to manage
        """
        self.plugin_name = plugin_name
        self.platform = get_platform_info()
        self.obs_exe = find_obs_executable()
        self.plugin_dir = get_plugin_install_path(plugin_name)
        self.binary_name = get_plugin_binary_name(plugin_name)

    def is_obs_installed(self) -> bool:
        """
        Check if OBS Studio is installed.

        Returns:
            True if OBS is found, False otherwise
        """
        return self.obs_exe is not None and self.obs_exe.exists()

    def get_obs_version(self) -> Optional[str]:
        """
        Get OBS Studio version.

        Returns:
            Version string, or None if not available
        """
        if not self.is_obs_installed():
            return None

        try:
            # Try running OBS with --version flag
            result = subprocess.run(
                [str(self.obs_exe), "--version"],
                capture_output=True,
                text=True,
                timeout=5
            )
            if result.returncode == 0:
                # Parse version from output
                # Output format varies, try to extract version number
                output = result.stdout.strip()
                if output:
                    # Common format: "OBS Studio 30.0.0"
                    parts = output.split()
                    for part in parts:
                        if part[0].isdigit():
                            return part
                return output
        except Exception:
            pass

        return None

    def is_plugin_installed(self) -> bool:
        """
        Check if the plugin is installed.

        Returns:
            True if plugin binary exists, False otherwise
        """
        binary_path = self.plugin_dir / self.binary_name
        return binary_path.exists()

    def get_plugin_path(self) -> Path:
        """
        Get the plugin installation directory.

        Returns:
            Path to plugin directory
        """
        return self.plugin_dir

    def get_plugin_binary_path(self) -> Path:
        """
        Get the full path to the plugin binary.

        Returns:
            Path to plugin binary file
        """
        return self.plugin_dir / self.binary_name

    def install_plugin(self, build_dir: Path) -> bool:
        """
        Install plugin from build directory.

        Args:
            build_dir: Path to CMake build directory

        Returns:
            True if successful, False otherwise
        """
        print(f"📦 Installing plugin from {build_dir}...")

        # Ensure plugin directory exists
        self.plugin_dir.mkdir(parents=True, exist_ok=True)

        # Find plugin binary in build directory
        if self.platform.is_windows:
            # Windows: look in Release/Debug subdirectories
            search_paths = [
                build_dir / "Release" / f"{self.plugin_name}.dll",
                build_dir / "Debug" / f"{self.plugin_name}.dll",
                build_dir / f"{self.plugin_name}.dll",
            ]
        elif self.platform.is_macos:
            # macOS: look for .so or .dylib
            search_paths = [
                build_dir / f"{self.plugin_name}.so",
                build_dir / f"{self.plugin_name}.dylib",
                build_dir / "Release" / f"{self.plugin_name}.so",
            ]
        else:  # Linux
            search_paths = [
                build_dir / f"{self.plugin_name}.so",
                build_dir / "lib" / f"{self.plugin_name}.so",
            ]

        source_binary = None
        for path in search_paths:
            if path.exists():
                source_binary = path
                break

        if not source_binary:
            print(f"❌ Plugin binary not found in {build_dir}")
            print(f"   Searched: {[str(p) for p in search_paths]}")
            return False

        # Copy binary to plugin directory
        dest_binary = self.get_plugin_binary_path()
        try:
            shutil.copy2(source_binary, dest_binary)
            print(f"✓ Copied {source_binary.name} to {dest_binary}")

            # Also copy any additional files (data, locale, etc.)
            data_dir = build_dir / "data"
            if data_dir.exists():
                dest_data = self.plugin_dir / "data"
                shutil.copytree(data_dir, dest_data, dirs_exist_ok=True)
                print(f"✓ Copied data directory")

            print(f"✅ Plugin installed successfully")
            return True

        except Exception as e:
            print(f"❌ Failed to install plugin: {e}")
            return False

    def uninstall_plugin(self) -> bool:
        """
        Uninstall the plugin.

        Returns:
            True if successful, False otherwise
        """
        print(f"🗑️  Uninstalling plugin from {self.plugin_dir}...")

        if not self.plugin_dir.exists():
            print("   Plugin not installed")
            return True

        try:
            shutil.rmtree(self.plugin_dir)
            print(f"✓ Plugin uninstalled")
            return True
        except Exception as e:
            print(f"❌ Failed to uninstall plugin: {e}")
            return False

    def verify_plugin_installation(self) -> Tuple[bool, List[str]]:
        """
        Verify plugin installation completeness.

        Returns:
            Tuple of (is_valid, list_of_issues)
        """
        issues = []

        # Check plugin directory exists
        if not self.plugin_dir.exists():
            issues.append(f"Plugin directory does not exist: {self.plugin_dir}")
            return False, issues

        # Check binary exists
        binary_path = self.get_plugin_binary_path()
        if not binary_path.exists():
            issues.append(f"Plugin binary not found: {binary_path}")
        else:
            # Check binary is readable and has reasonable size
            try:
                size = binary_path.stat().st_size
                if size < 1024:  # Less than 1KB is suspicious
                    issues.append(f"Plugin binary suspiciously small: {size} bytes")
            except Exception as e:
                issues.append(f"Cannot read plugin binary: {e}")

        # Check for data files (if expected)
        # This depends on your plugin structure

        return len(issues) == 0, issues

    def get_obs_log_dir(self) -> Path:
        """
        Get OBS log directory for the current platform.

        Returns:
            Path to OBS log directory
        """
        if self.platform.is_windows:
            appdata = os.getenv('APPDATA', '')
            return Path(appdata) / "obs-studio" / "logs"
        elif self.platform.is_macos:
            return Path.home() / "Library" / "Application Support" / "obs-studio" / "logs"
        else:  # Linux
            return Path.home() / ".config" / "obs-studio" / "logs"

    def get_latest_log_file(self) -> Optional[Path]:
        """
        Get the most recent OBS log file.

        Returns:
            Path to latest log file, or None if not found
        """
        log_dir = self.get_obs_log_dir()
        if not log_dir.exists():
            return None

        # Find latest log file
        log_files = list(log_dir.glob("*.txt"))
        if not log_files:
            return None

        # Sort by modification time, newest first
        log_files.sort(key=lambda p: p.stat().st_mtime, reverse=True)
        return log_files[0]

    def read_log_file(self, log_file: Optional[Path] = None) -> Optional[str]:
        """
        Read OBS log file contents.

        Args:
            log_file: Path to log file (default: latest)

        Returns:
            Log file contents, or None if not found
        """
        if log_file is None:
            log_file = self.get_latest_log_file()

        if log_file is None or not log_file.exists():
            return None

        try:
            with open(log_file, 'r', encoding='utf-8', errors='ignore') as f:
                return f.read()
        except Exception:
            return None

    def search_log_for_plugin(self, log_file: Optional[Path] = None) -> Dict[str, any]:
        """
        Search log file for plugin-related messages.

        Args:
            log_file: Path to log file (default: latest)

        Returns:
            Dictionary with plugin load status and messages
        """
        log_content = self.read_log_file(log_file)
        if not log_content:
            return {
                "found_log": False,
                "plugin_loaded": False,
                "messages": []
            }

        # Search for plugin mentions
        plugin_messages = []
        plugin_loaded = False
        plugin_name_lower = self.plugin_name.lower()

        for line in log_content.split('\n'):
            line_lower = line.lower()
            if plugin_name_lower in line_lower:
                plugin_messages.append(line.strip())
                if "loaded" in line_lower or "initialized" in line_lower:
                    plugin_loaded = True

        return {
            "found_log": True,
            "plugin_loaded": plugin_loaded,
            "messages": plugin_messages
        }

    def launch_obs(self,
                   portable: bool = False,
                   verbose: bool = True,
                   additional_args: Optional[List[str]] = None,
                   timeout: Optional[int] = None) -> subprocess.Popen:
        """
        Launch OBS Studio.

        Args:
            portable: Use portable mode
            verbose: Enable verbose logging
            additional_args: Additional command-line arguments
            timeout: Maximum time to run (seconds), None for indefinite

        Returns:
            Popen process object

        Raises:
            RuntimeError: If OBS is not installed
        """
        if not self.is_obs_installed():
            raise RuntimeError("OBS Studio is not installed")

        args = [str(self.obs_exe)]

        if portable:
            args.append("--portable")

        if verbose:
            args.append("--verbose")

        if additional_args:
            args.extend(additional_args)

        print(f"🚀 Launching OBS: {' '.join(args)}")

        process = subprocess.Popen(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        # If timeout specified, wait and then terminate
        if timeout:
            try:
                process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                print(f"⏱️  Timeout reached ({timeout}s), terminating OBS...")
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()

        return process

    def test_plugin_load(self, timeout: int = 30) -> Tuple[bool, str]:
        """
        Test if plugin loads successfully in OBS.

        Args:
            timeout: Maximum time to run OBS (seconds)

        Returns:
            Tuple of (success, message)
        """
        if not self.is_obs_installed():
            return False, "OBS Studio is not installed"

        if not self.is_plugin_installed():
            return False, f"Plugin {self.plugin_name} is not installed"

        print(f"🧪 Testing plugin load (timeout: {timeout}s)...")

        # Clear old logs or note current log file
        old_log = self.get_latest_log_file()

        # Launch OBS briefly
        try:
            process = self.launch_obs(
                verbose=True,
                additional_args=["--minimize-to-tray"],
                timeout=timeout
            )

            # Wait for process to complete
            returncode = process.wait()

            # Get new log file
            time.sleep(1)  # Give filesystem a moment
            new_log = self.get_latest_log_file()

            # Check if plugin loaded
            log_result = self.search_log_for_plugin(new_log)

            if log_result["plugin_loaded"]:
                return True, f"Plugin loaded successfully (found {len(log_result['messages'])} log entries)"
            else:
                if log_result["messages"]:
                    return False, f"Plugin mentioned in logs but not loaded: {log_result['messages']}"
                else:
                    return False, "Plugin not mentioned in OBS logs"

        except Exception as e:
            return False, f"Error testing plugin: {e}"


def check_obs_requirements() -> Dict[str, any]:
    """
    Check if OBS testing requirements are met.

    Returns:
        Dictionary with requirement status
    """
    manager = OBSManager()

    return {
        "obs_installed": manager.is_obs_installed(),
        "obs_path": str(manager.obs_exe) if manager.obs_exe else None,
        "obs_version": manager.get_obs_version(),
        "plugin_installed": manager.is_plugin_installed(),
        "plugin_path": str(manager.get_plugin_path()),
        "plugin_binary": str(manager.get_plugin_binary_path()),
        "platform": str(manager.platform)
    }


def print_obs_status():
    """Print comprehensive OBS status for debugging."""
    status = check_obs_requirements()

    print("\n📊 OBS Testing Environment Status")
    print("=" * 50)
    print(f"Platform: {status['platform']}")
    print(f"OBS Installed: {'✅' if status['obs_installed'] else '❌'}")
    if status['obs_path']:
        print(f"OBS Path: {status['obs_path']}")
    if status['obs_version']:
        print(f"OBS Version: {status['obs_version']}")
    print(f"\nPlugin Installed: {'✅' if status['plugin_installed'] else '❌'}")
    print(f"Plugin Directory: {status['plugin_path']}")
    print(f"Plugin Binary: {status['plugin_binary']}")

    if status['plugin_installed']:
        manager = OBSManager()
        valid, issues = manager.verify_plugin_installation()
        if valid:
            print("Plugin Verification: ✅ Valid")
        else:
            print("Plugin Verification: ❌ Issues found:")
            for issue in issues:
                print(f"  - {issue}")

    print("=" * 50)


if __name__ == "__main__":
    """CLI entry point for testing."""
    import argparse

    parser = argparse.ArgumentParser(description="OBS Helper Utilities")
    parser.add_argument("action",
                        choices=["status", "install", "uninstall", "verify", "test-load"],
                        help="Action to perform")
    parser.add_argument("--build-dir", type=Path,
                        help="Build directory (for install action)")
    parser.add_argument("--timeout", type=int, default=30,
                        help="Timeout for test-load action (default: 30s)")

    args = parser.parse_args()

    manager = OBSManager()

    if args.action == "status":
        print_obs_status()

    elif args.action == "install":
        if not args.build_dir:
            print("❌ --build-dir required for install action")
            sys.exit(1)
        success = manager.install_plugin(args.build_dir)
        sys.exit(0 if success else 1)

    elif args.action == "uninstall":
        success = manager.uninstall_plugin()
        sys.exit(0 if success else 1)

    elif args.action == "verify":
        valid, issues = manager.verify_plugin_installation()
        if valid:
            print("✅ Plugin installation is valid")
            sys.exit(0)
        else:
            print("❌ Plugin installation has issues:")
            for issue in issues:
                print(f"  - {issue}")
            sys.exit(1)

    elif args.action == "test-load":
        success, message = manager.test_plugin_load(timeout=args.timeout)
        print(f"{'✅' if success else '❌'} {message}")
        sys.exit(0 if success else 1)
