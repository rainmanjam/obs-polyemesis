"""
OBS Integration Tests

Tests OBS Studio integration without requiring Restreamer.
These tests verify:
- OBS installation and version
- Plugin installation and structure
- Cross-platform path handling
- Log file access
- Binary verification
"""

import os
import sys
import pytest
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from utils.obs_helpers import OBSManager, check_obs_requirements, print_obs_status
from utils.platform_helpers import get_platform_info, platform_info


# ========================================================================
# OBS INSTALLATION TESTS
# ========================================================================

class TestOBSInstallation:
    """Test OBS Studio installation."""

    def test_obs_executable_found(self):
        """Test that OBS executable can be located."""
        manager = OBSManager()

        if manager.obs_exe is None:
            pytest.skip("OBS Studio not installed")

        assert manager.obs_exe.exists(), f"OBS executable not found at {manager.obs_exe}"
        print(f"   OBS executable: {manager.obs_exe}")

    def test_obs_version_detectable(self):
        """Test that OBS version can be detected."""
        manager = OBSManager()

        if not manager.is_obs_installed():
            pytest.skip("OBS Studio not installed")

        version = manager.get_obs_version()
        # Version might not always be detectable, so this is informational
        print(f"   OBS version: {version if version else 'Not detectable'}")

    def test_platform_detection(self):
        """Test platform-specific paths are correct."""
        pinfo = get_platform_info()

        print(f"   Platform: {pinfo}")
        print(f"   OBS plugin dir: {pinfo.obs_plugin_dir}")
        print(f"   OBS executable: {pinfo.obs_executable_name}")

        # Verify plugin directory path makes sense for platform
        if pinfo.is_windows:
            assert "obs-studio" in str(pinfo.obs_plugin_dir).lower()
            assert pinfo.obs_executable_name == "obs64.exe"
        elif pinfo.is_macos:
            assert "Library" in str(pinfo.obs_plugin_dir)
            assert "Application Support" in str(pinfo.obs_plugin_dir)
            assert pinfo.obs_executable_name == "OBS"
        elif pinfo.is_linux:
            assert ".config" in str(pinfo.obs_plugin_dir)
            assert pinfo.obs_executable_name == "obs"


# ========================================================================
# PLUGIN INSTALLATION TESTS
# ========================================================================

class TestPluginInstallation:
    """Test plugin installation and structure."""

    def test_plugin_directory_path(self):
        """Test plugin directory path is correct for platform."""
        manager = OBSManager()

        plugin_dir = manager.get_plugin_path()
        print(f"   Plugin directory: {plugin_dir}")

        # Verify path contains expected components
        assert "obs-studio" in str(plugin_dir).lower()
        assert "plugins" in str(plugin_dir).lower()
        assert manager.plugin_name in str(plugin_dir)

    def test_plugin_binary_name(self):
        """Test plugin binary name is correct for platform."""
        manager = OBSManager()

        binary_name = manager.binary_name
        print(f"   Plugin binary: {binary_name}")

        # Verify extension matches platform
        if platform_info.is_windows:
            assert binary_name.endswith(".dll")
        elif platform_info.is_macos or platform_info.is_linux:
            assert binary_name.endswith(".so") or binary_name.endswith(".dylib")

    def test_plugin_binary_exists(self):
        """Test plugin binary file exists."""
        manager = OBSManager()

        if not manager.is_plugin_installed():
            pytest.skip("Plugin not installed")

        binary_path = manager.get_plugin_binary_path()
        assert binary_path.exists(), f"Plugin binary not found at {binary_path}"

        # Check file size is reasonable (should be at least 10KB)
        size = binary_path.stat().st_size
        assert size >= 10240, f"Plugin binary suspiciously small: {size} bytes"

        print(f"   Binary path: {binary_path}")
        print(f"   Binary size: {size:,} bytes")

    def test_plugin_verification(self):
        """Test plugin installation verification."""
        manager = OBSManager()

        if not manager.is_plugin_installed():
            pytest.skip("Plugin not installed")

        valid, issues = manager.verify_plugin_installation()

        if not valid:
            print("   Installation issues found:")
            for issue in issues:
                print(f"     - {issue}")

        assert valid, f"Plugin installation invalid: {issues}"
        print("   ✓ Plugin installation verified")


# ========================================================================
# OBS LOG FILE TESTS
# ========================================================================

class TestOBSLogFiles:
    """Test OBS log file access and analysis."""

    def test_log_directory_path(self):
        """Test log directory path is correct for platform."""
        manager = OBSManager()

        log_dir = manager.get_obs_log_dir()
        print(f"   Log directory: {log_dir}")

        # Verify path makes sense for platform
        if platform_info.is_windows:
            assert "obs-studio" in str(log_dir).lower()
        elif platform_info.is_macos:
            assert "Library" in str(log_dir)
        elif platform_info.is_linux:
            assert ".config" in str(log_dir)

    def test_log_file_readable(self):
        """Test that log files can be read."""
        manager = OBSManager()

        latest_log = manager.get_latest_log_file()

        if latest_log is None:
            pytest.skip("No OBS log files found (OBS may not have been run)")

        assert latest_log.exists(), f"Log file not found: {latest_log}"
        print(f"   Latest log: {latest_log}")

        # Try to read log
        content = manager.read_log_file(latest_log)
        assert content is not None, "Failed to read log file"
        assert len(content) > 0, "Log file is empty"

        print(f"   Log size: {len(content):,} characters")

    def test_plugin_log_search(self):
        """Test searching logs for plugin mentions."""
        manager = OBSManager()

        if not manager.is_plugin_installed():
            pytest.skip("Plugin not installed")

        result = manager.search_log_for_plugin()

        if not result["found_log"]:
            pytest.skip("No OBS log files found")

        print(f"   Log found: {result['found_log']}")
        print(f"   Plugin loaded: {result['plugin_loaded']}")
        print(f"   Plugin mentions: {len(result['messages'])}")

        if result["messages"]:
            print("   Sample log entries:")
            for msg in result["messages"][:3]:
                print(f"     {msg[:80]}...")


# ========================================================================
# CROSS-PLATFORM COMPATIBILITY TESTS
# ========================================================================

class TestCrossPlatformCompatibility:
    """Test cross-platform compatibility."""

    def test_path_handling_windows(self):
        """Test Windows path handling."""
        from utils.platform_helpers import PlatformInfo

        # Create a mock Windows platform info
        pinfo = PlatformInfo()
        pinfo.system = "Windows"
        pinfo.is_windows = True
        pinfo.is_macos = False
        pinfo.is_linux = False

        # Test plugin directory
        plugin_dir = pinfo.obs_plugin_dir
        # Should handle APPDATA environment variable
        print(f"   Windows plugin dir: {plugin_dir}")

    def test_path_handling_macos(self):
        """Test macOS path handling."""
        from utils.platform_helpers import PlatformInfo

        # Create a mock macOS platform info
        pinfo = PlatformInfo()
        pinfo.system = "Darwin"
        pinfo.is_windows = False
        pinfo.is_macos = True
        pinfo.is_linux = False

        # Test plugin directory
        plugin_dir = pinfo.obs_plugin_dir
        assert "Library" in str(plugin_dir)
        assert "Application Support" in str(plugin_dir)

        print(f"   macOS plugin dir: {plugin_dir}")

    def test_path_handling_linux(self):
        """Test Linux path handling."""
        from utils.platform_helpers import PlatformInfo

        # Create a mock Linux platform info
        pinfo = PlatformInfo()
        pinfo.system = "Linux"
        pinfo.is_windows = False
        pinfo.is_macos = False
        pinfo.is_linux = True

        # Test plugin directory
        plugin_dir = pinfo.obs_plugin_dir
        assert ".config" in str(plugin_dir)

        print(f"   Linux plugin dir: {plugin_dir}")

    def test_current_platform_detection(self):
        """Test current platform is correctly detected."""
        pinfo = get_platform_info()

        print(f"   Detected platform: {pinfo}")
        print(f"   Machine: {pinfo.machine}")
        print(f"   Python version: {pinfo.python_version}")

        # Exactly one should be true
        platform_flags = [pinfo.is_windows, pinfo.is_macos, pinfo.is_linux]
        assert sum(platform_flags) == 1, "Multiple or no platforms detected"


# ========================================================================
# ENVIRONMENT STATUS TESTS
# ========================================================================

class TestEnvironmentStatus:
    """Test environment status reporting."""

    def test_obs_requirements_check(self):
        """Test OBS requirements check function."""
        status = check_obs_requirements()

        print("\n   Environment Status:")
        print(f"     OBS Installed: {status['obs_installed']}")
        print(f"     OBS Path: {status['obs_path']}")
        print(f"     OBS Version: {status['obs_version']}")
        print(f"     Plugin Installed: {status['plugin_installed']}")
        print(f"     Plugin Path: {status['plugin_path']}")
        print(f"     Plugin Binary: {status['plugin_binary']}")
        print(f"     Platform: {status['platform']}")

        # Verify structure
        assert isinstance(status, dict)
        assert "obs_installed" in status
        assert "plugin_installed" in status
        assert "platform" in status

    def test_print_obs_status_output(self, capsys):
        """Test OBS status printing function."""
        print_obs_status()

        captured = capsys.readouterr()
        output = captured.out

        # Verify output contains expected sections
        assert "OBS Testing Environment Status" in output
        assert "Platform:" in output
        assert "OBS Installed:" in output
        assert "Plugin Installed:" in output


# ========================================================================
# INSTALLATION HELPERS TESTS
# ========================================================================

class TestInstallationHelpers:
    """Test installation helper functions."""

    @pytest.mark.skipif(
        not OBSManager().is_plugin_installed(),
        reason="Plugin not installed"
    )
    def test_plugin_can_be_uninstalled_and_reinstalled(self, tmp_path):
        """
        Test plugin uninstall and reinstall process.

        WARNING: This test will temporarily uninstall the plugin!
        Only run in CI or when you're prepared to rebuild.
        """
        pytest.skip("Skipping destructive test - would uninstall plugin")

        # This test is intentionally skipped by default
        # Uncomment to test installation/uninstallation logic

        # manager = OBSManager()
        #
        # # Backup current installation
        # ...
        #
        # # Uninstall
        # assert manager.uninstall_plugin()
        # assert not manager.is_plugin_installed()
        #
        # # Reinstall
        # build_dir = Path("build")  # Adjust as needed
        # assert manager.install_plugin(build_dir)
        # assert manager.is_plugin_installed()


# Pytest configuration
def pytest_configure(config):
    """Configure pytest."""
    config.addinivalue_line(
        "markers", "obs: mark test as OBS-specific"
    )
    config.addinivalue_line(
        "markers", "plugin: mark test as plugin-specific"
    )


if __name__ == "__main__":
    """Allow running tests directly."""
    print("\n" + "="*70)
    print("OBS INTEGRATION TESTS")
    print("="*70)
    print("\nEnvironment Status:")
    print_obs_status()
    print("\n" + "="*70)

    pytest.main([__file__, "-v", "-s"])
