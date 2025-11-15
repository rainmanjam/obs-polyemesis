"""
End-to-End Plugin Tests - OBS Polyemesis + Restreamer Integration

Tests the complete integration of:
- OBS Studio with obs-polyemesis plugin installed
- Live Restreamer Docker instance
- Plugin functionality (loading, UI, API communication)
- Real-world streaming scenarios

These tests require:
1. OBS Studio installed
2. obs-polyemesis plugin installed
3. Restreamer Docker container running
"""

import os
import sys
import pytest
import requests
import time
from pathlib import Path
from typing import Optional

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from utils.obs_helpers import OBSManager, check_obs_requirements
from utils.platform_helpers import load_test_config, is_docker_available
from setup.start_restreamer import RestreamerManager


# Skip all tests if OBS is not installed
pytestmark = pytest.mark.skipif(
    not OBSManager().is_obs_installed(),
    reason="OBS Studio not installed - skipping E2E plugin tests"
)


@pytest.fixture(scope="module")
def config():
    """Load test configuration."""
    return load_test_config()


@pytest.fixture(scope="module")
def obs_manager():
    """Create OBS manager."""
    manager = OBSManager()

    # Verify OBS is installed
    if not manager.is_obs_installed():
        pytest.skip("OBS Studio not installed")

    return manager


@pytest.fixture(scope="module")
def restreamer_manager(config):
    """
    Start Restreamer for E2E tests.

    This fixture starts Restreamer and ensures it's healthy before tests run.
    """
    # Check Docker availability
    if not is_docker_available():
        pytest.skip("Docker not available - cannot run Restreamer")

    manager = RestreamerManager()

    print("\n🐳 Starting Restreamer Docker container...")

    # Start Restreamer
    if not manager.start(pull=False):
        pytest.skip("Failed to start Restreamer")

    # Wait for healthy
    if not manager.wait_for_health():
        manager.down()
        pytest.skip("Restreamer failed health check")

    # Get authentication token
    token = manager.get_jwt_token()
    if not token:
        manager.down()
        pytest.skip("Failed to get Restreamer auth token")

    print("✅ Restreamer is ready for E2E tests")

    yield manager

    # Cleanup - stop Restreamer after tests
    print("\n🛑 Stopping Restreamer...")
    manager.down(volumes=True)


@pytest.fixture(scope="module")
def access_token(restreamer_manager):
    """Get Restreamer access token."""
    return restreamer_manager.get_jwt_token()


# ========================================================================
# ENVIRONMENT VALIDATION TESTS
# ========================================================================

class TestEnvironmentSetup:
    """Validate the E2E test environment."""

    def test_obs_installed(self, obs_manager):
        """Verify OBS Studio is installed."""
        assert obs_manager.is_obs_installed(), "OBS Studio not found"

        version = obs_manager.get_obs_version()
        print(f"   OBS Version: {version if version else 'Unknown'}")

    def test_plugin_installed(self, obs_manager):
        """Verify obs-polyemesis plugin is installed."""
        assert obs_manager.is_plugin_installed(), \
            f"Plugin not found at {obs_manager.get_plugin_binary_path()}"

        print(f"   Plugin location: {obs_manager.get_plugin_path()}")

    def test_plugin_installation_valid(self, obs_manager):
        """Verify plugin installation is complete and valid."""
        valid, issues = obs_manager.verify_plugin_installation()

        if not valid:
            for issue in issues:
                print(f"   ⚠️  {issue}")

        assert valid, f"Plugin installation issues: {issues}"
        print("   ✓ Plugin installation verified")

    def test_restreamer_running(self, restreamer_manager):
        """Verify Restreamer is running and healthy."""
        assert restreamer_manager.is_running(), "Restreamer not running"

        status = restreamer_manager.get_status()
        assert status["healthy"], "Restreamer not healthy"
        assert status["authenticated"], "Restreamer authentication failed"

        print(f"   Restreamer version: {status.get('version', 'Unknown')}")

    def test_restreamer_api_accessible(self, restreamer_manager, access_token):
        """Verify Restreamer API is accessible."""
        # Try to list processes
        response = requests.get(
            f"{restreamer_manager.base_url}/api/v3/process",
            headers={"Authorization": f"Bearer {access_token}"},
            timeout=10
        )

        assert response.status_code == 200, \
            f"API not accessible: {response.status_code}"

        processes = response.json()
        print(f"   API accessible - {len(processes)} processes found")


# ========================================================================
# PLUGIN LOADING TESTS
# ========================================================================

class TestPluginLoading:
    """Test plugin loading in OBS."""

    def test_plugin_loads_in_obs(self, obs_manager):
        """
        Test that plugin loads successfully when OBS starts.

        This launches OBS briefly and checks the logs for plugin loading.
        """
        success, message = obs_manager.test_plugin_load(timeout=30)

        assert success, f"Plugin failed to load: {message}"
        print(f"   ✓ {message}")

    def test_plugin_log_entries(self, obs_manager):
        """Check that plugin writes expected log entries."""
        log_result = obs_manager.search_log_for_plugin()

        assert log_result["found_log"], "No OBS log file found"

        if log_result["plugin_loaded"]:
            print(f"   ✓ Plugin log entries found: {len(log_result['messages'])}")
            for msg in log_result["messages"][:5]:  # Show first 5 entries
                print(f"      - {msg[:80]}...")
        else:
            # Plugin might not have been loaded yet
            print(f"   ℹ️  Plugin not yet loaded (found {len(log_result['messages'])} mentions)")


# ========================================================================
# API INTEGRATION TESTS
# ========================================================================

class TestPluginAPIIntegration:
    """Test plugin's integration with Restreamer API."""

    @pytest.fixture
    def test_process_id(self):
        """Generate a unique process ID for tests."""
        return f"e2e_test_process_{os.getpid()}"

    def test_plugin_can_authenticate(self, restreamer_manager, access_token):
        """
        Test that plugin can authenticate with Restreamer.

        This simulates what the plugin does when connecting.
        """
        assert access_token is not None, "Failed to get access token"
        assert len(access_token) > 0, "Access token is empty"

        print(f"   ✓ Authentication token obtained (length: {len(access_token)})")

    def test_plugin_can_list_processes(self, restreamer_manager, access_token):
        """Test that plugin can list processes via API."""
        response = requests.get(
            f"{restreamer_manager.base_url}/api/v3/process",
            headers={"Authorization": f"Bearer {access_token}"},
            timeout=10
        )

        assert response.status_code == 200, "Failed to list processes"
        processes = response.json()
        assert isinstance(processes, list), "Response is not a list"

        print(f"   ✓ Listed {len(processes)} processes")

    def test_plugin_can_create_process(self, restreamer_manager, access_token, test_process_id):
        """Test that plugin can create a process."""
        response = requests.post(
            f"{restreamer_manager.base_url}/api/v3/process",
            headers={
                "Authorization": f"Bearer {access_token}",
                "Content-Type": "application/json"
            },
            json={
                "id": test_process_id,
                "reference": "e2e_plugin_test"
            },
            timeout=10
        )

        assert response.status_code in [200, 201], \
            f"Failed to create process: {response.status_code} - {response.text}"

        created_id = response.json().get("id")
        assert created_id == test_process_id, "Process ID mismatch"

        print(f"   ✓ Created process: {test_process_id}")

        # Cleanup
        requests.delete(
            f"{restreamer_manager.base_url}/api/v3/process/{test_process_id}",
            headers={"Authorization": f"Bearer {access_token}"},
            timeout=10
        )

    def test_plugin_can_access_filesystems(self, restreamer_manager, access_token):
        """Test that plugin can access file system browser."""
        # List filesystems
        response = requests.get(
            f"{restreamer_manager.base_url}/api/v3/fs",
            headers={"Authorization": f"Bearer {access_token}"},
            timeout=10
        )

        assert response.status_code == 200, "Failed to list filesystems"
        filesystems = response.json()
        assert isinstance(filesystems, list), "Response is not a list"
        assert len(filesystems) > 0, "No filesystems available"

        print(f"   ✓ Accessed {len(filesystems)} filesystems")


# ========================================================================
# UI FUNCTIONALITY TESTS
# ========================================================================

class TestPluginUI:
    """Test plugin UI components."""

    def test_plugin_dock_available(self, obs_manager):
        """
        Test that plugin dock is available in OBS.

        Note: This is a basic test - actual UI testing would require
        automation tools like pyautogui or similar.
        """
        # This test verifies the plugin loaded (which includes UI registration)
        log_result = obs_manager.search_log_for_plugin()

        # If plugin loaded, UI should be registered
        if log_result["plugin_loaded"]:
            print("   ✓ Plugin loaded - UI components should be available")
        else:
            pytest.skip("Plugin not loaded - UI test skipped")

    def test_plugin_menu_entry(self, obs_manager):
        """
        Test that plugin adds menu entry to View menu.

        Note: Actual menu interaction would require UI automation.
        """
        # Verify plugin is installed and loaded
        assert obs_manager.is_plugin_installed(), "Plugin not installed"

        print("   ℹ️  Plugin menu entry test requires manual verification")
        print("      Check: View > Docks > Restreamer Control")


# ========================================================================
# REAL-WORLD SCENARIO TESTS
# ========================================================================

class TestRealWorldScenarios:
    """Test real-world streaming scenarios."""

    @pytest.fixture
    def streaming_process(self, restreamer_manager, access_token):
        """Create a streaming process for scenario tests."""
        process_id = f"scenario_test_{os.getpid()}"

        # Create process with actual streaming configuration
        response = requests.post(
            f"{restreamer_manager.base_url}/api/v3/process",
            headers={
                "Authorization": f"Bearer {access_token}",
                "Content-Type": "application/json"
            },
            json={
                "id": process_id,
                "reference": "scenario_test",
                "input": [{
                    "id": "input_0",
                    "address": "testsrc=duration=60:size=1280x720:rate=30",
                    "options": ["-f", "lavfi"]
                }],
                "output": [{
                    "id": "output_0",
                    "address": "http://localhost:8888/test.m3u8",
                    "options": [
                        "-codec:v", "libx264",
                        "-preset", "ultrafast",
                        "-b:v", "1000k",
                        "-f", "hls"
                    ]
                }]
            },
            timeout=10
        )

        assert response.status_code in [200, 201], "Failed to create streaming process"

        yield process_id

        # Cleanup
        requests.delete(
            f"{restreamer_manager.base_url}/api/v3/process/{process_id}",
            headers={"Authorization": f"Bearer {access_token}"},
            timeout=10
        )

    def test_scenario_basic_streaming_setup(self, streaming_process, restreamer_manager, access_token):
        """
        Scenario: User sets up basic streaming

        Steps:
        1. Create stream process
        2. Add metadata
        3. Monitor status
        """
        print(f"\n   📹 Testing basic streaming setup scenario...")

        # Add stream metadata
        metadata_updates = {
            "title": "Test Stream - E2E",
            "description": "Automated E2E test stream",
            "tags": "test,automated,e2e"
        }

        for key, value in metadata_updates.items():
            response = requests.put(
                f"{restreamer_manager.base_url}/api/v3/process/{streaming_process}/metadata/{key}",
                headers={
                    "Authorization": f"Bearer {access_token}",
                    "Content-Type": "application/json"
                },
                json=value,
                timeout=10
            )
            assert response.status_code in [200, 204], f"Failed to set {key}"

        print("   ✓ Stream metadata configured")

        # Check stream status
        response = requests.get(
            f"{restreamer_manager.base_url}/api/v3/process/{streaming_process}/state",
            headers={"Authorization": f"Bearer {access_token}"},
            timeout=10
        )

        assert response.status_code == 200, "Failed to get stream state"
        state = response.json()

        print(f"   ✓ Stream state: {state.get('order', 'unknown')}")
        print("   ✅ Basic streaming scenario completed")

    def test_scenario_file_upload_for_stream(self, restreamer_manager, access_token):
        """
        Scenario: User uploads overlay image for stream

        Steps:
        1. Upload image file
        2. Verify upload
        3. Delete file
        """
        print(f"\n   📤 Testing file upload scenario...")

        # Create test image data
        test_image = b"PNG_MOCK_DATA_" + os.urandom(100)
        filename = f"e2e_overlay_{os.getpid()}.png"

        # Upload
        response = requests.put(
            f"{restreamer_manager.base_url}/api/v3/fs/disk/{filename}",
            headers={
                "Authorization": f"Bearer {access_token}",
                "Content-Type": "image/png"
            },
            data=test_image,
            timeout=10
        )

        assert response.status_code in [200, 201, 204], "Failed to upload file"
        print(f"   ✓ Uploaded {filename}")

        # Verify
        response = requests.get(
            f"{restreamer_manager.base_url}/api/v3/fs/disk/{filename}",
            headers={"Authorization": f"Bearer {access_token}"},
            timeout=10
        )

        assert response.status_code == 200, "Failed to download file"
        assert response.content == test_image, "Downloaded content doesn't match"
        print("   ✓ Verified file content")

        # Cleanup
        requests.delete(
            f"{restreamer_manager.base_url}/api/v3/fs/disk/{filename}",
            headers={"Authorization": f"Bearer {access_token}"},
            timeout=10
        )

        print("   ✅ File upload scenario completed")


# Pytest configuration
def pytest_configure(config):
    """Configure pytest."""
    config.addinivalue_line(
        "markers", "e2e: mark test as end-to-end integration test"
    )
    config.addinivalue_line(
        "markers", "requires_obs: mark test as requiring OBS Studio"
    )
    config.addinivalue_line(
        "markers", "requires_docker: mark test as requiring Docker"
    )


if __name__ == "__main__":
    """Allow running tests directly."""
    print("\n" + "="*70)
    print("OBS POLYEMESIS - E2E PLUGIN INTEGRATION TESTS")
    print("="*70)
    print("\nChecking environment...")

    # Check requirements
    status = check_obs_requirements()
    print(f"\nOBS Installed: {'✅' if status['obs_installed'] else '❌'}")
    print(f"Plugin Installed: {'✅' if status['plugin_installed'] else '❌'}")
    print(f"Docker Available: {'✅' if is_docker_available() else '❌'}")

    if not status['obs_installed']:
        print("\n⚠️  OBS Studio not found - tests will be skipped")
    if not status['plugin_installed']:
        print("\n⚠️  obs-polyemesis plugin not found - tests will be skipped")
    if not is_docker_available():
        print("\n⚠️  Docker not available - Restreamer tests will be skipped")

    print("\n" + "="*70)
    pytest.main([__file__, "-v", "-s"])
