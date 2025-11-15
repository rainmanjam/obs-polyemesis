"""
API Integration Tests for OBS Polyemesis

Tests all 4 newly implemented features against live Restreamer instance:
- Feature #7: Metadata Storage
- Feature #8: File System Browser
- Feature #9: Dynamic Output Management
- Feature #10: Playout Management
"""

import os
import sys
import pytest
import requests
import json
import tempfile
from pathlib import Path
from typing import Optional

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from utils.platform_helpers import load_test_config
from setup.start_restreamer import RestreamerManager


# Test configuration
@pytest.fixture(scope="module")
def config():
    """Load test configuration."""
    return load_test_config()


@pytest.fixture(scope="module")
def base_url(config):
    """Get base API URL."""
    return config["api"]["base_url"]


@pytest.fixture(scope="module")
def api_version(config):
    """Get API version."""
    return config["api"]["version"]


@pytest.fixture(scope="module")
def credentials(config):
    """Get authentication credentials."""
    return config["credentials"]


@pytest.fixture(scope="module")
def api_base(base_url, api_version):
    """Get full API base URL."""
    return f"{base_url}/api/{api_version}"


class RestreamerAPI:
    """Helper class for Restreamer API interactions."""

    def __init__(self, base_url: str, api_base: str, credentials: dict, config: dict):
        self.base_url = base_url
        self.api_base = api_base
        self.username = credentials["username"]
        self.password = credentials["password"]
        self.timeout = config["api"]["timeout"]
        self.access_token: Optional[str] = None

    def wait_for_ready(self, max_attempts: int = 30, interval: int = 2) -> bool:
        """
        Wait for Restreamer to become ready.

        Args:
            max_attempts: Maximum number of attempts
            interval: Seconds between attempts

        Returns:
            True if ready, False if timeout
        """
        import time

        print(f"⏳ Waiting for Restreamer at {self.base_url}...")

        for attempt in range(1, max_attempts + 1):
            try:
                response = requests.get(
                    f"{self.api_base}/about",
                    timeout=5
                )
                # 200 or 401 both mean server is ready
                if response.status_code in [200, 401]:
                    print(f"✅ Restreamer is ready! (HTTP {response.status_code})")
                    return True
            except requests.exceptions.RequestException:
                pass

            if attempt % 5 == 0:
                print(f"   Still waiting... (attempt {attempt}/{max_attempts})")

            time.sleep(interval)

        print("❌ Restreamer failed to become ready")
        return False

    def authenticate(self) -> bool:
        """
        Authenticate with Restreamer and get access token.

        Returns:
            True if successful, False otherwise
        """
        try:
            response = requests.post(
                f"{self.base_url}/api/login",
                json={
                    "username": self.username,
                    "password": self.password
                },
                timeout=self.timeout
            )

            if response.status_code == 200:
                data = response.json()
                self.access_token = data.get("access_token")
                if self.access_token:
                    print(f"✅ Authentication successful")
                    return True

            print(f"❌ Authentication failed: {response.status_code} - {response.text}")
            return False

        except requests.exceptions.RequestException as e:
            print(f"❌ Authentication error: {e}")
            return False

    def get_headers(self) -> dict:
        """Get headers with authorization token."""
        if not self.access_token:
            raise RuntimeError("Not authenticated - call authenticate() first")
        return {
            "Authorization": f"Bearer {self.access_token}",
            "Content-Type": "application/json"
        }

    def get(self, endpoint: str, **kwargs) -> requests.Response:
        """Make authenticated GET request."""
        return requests.get(
            f"{self.api_base}/{endpoint}",
            headers=self.get_headers(),
            timeout=self.timeout,
            **kwargs
        )

    def post(self, endpoint: str, **kwargs) -> requests.Response:
        """Make authenticated POST request."""
        return requests.post(
            f"{self.api_base}/{endpoint}",
            headers=self.get_headers(),
            timeout=self.timeout,
            **kwargs
        )

    def put(self, endpoint: str, **kwargs) -> requests.Response:
        """Make authenticated PUT request."""
        return requests.put(
            f"{self.api_base}/{endpoint}",
            headers=self.get_headers(),
            timeout=self.timeout,
            **kwargs
        )

    def delete(self, endpoint: str, **kwargs) -> requests.Response:
        """Make authenticated DELETE request."""
        return requests.delete(
            f"{self.api_base}/{endpoint}",
            headers=self.get_headers(),
            timeout=self.timeout,
            **kwargs
        )


@pytest.fixture(scope="module")
def api(base_url, api_base, credentials, config):
    """Create and authenticate API client."""
    client = RestreamerAPI(base_url, api_base, credentials, config)

    # Wait for Restreamer to be ready
    assert client.wait_for_ready(), "Restreamer failed to become ready"

    # Authenticate
    assert client.authenticate(), "Authentication failed"

    return client


@pytest.fixture(scope="module")
def test_process(api):
    """
    Create a test process for testing.

    Yields the process ID and cleans up afterwards.
    """
    process_id = f"test_process_{os.getpid()}"

    # Create test process
    process_config = {
        "id": process_id,
        "reference": "test_ref",
        "input": [{
            "id": "input_0",
            "address": "testsrc=duration=3600:size=1280x720:rate=30",
            "options": ["-f", "lavfi"]
        }],
        "output": [{
            "id": "output_0",
            "address": "http://localhost:8888/test.m3u8",
            "options": [
                "-codec:v", "libx264",
                "-preset", "ultrafast",
                "-b:v", "1000k",
                "-codec:a", "aac",
                "-b:a", "128k",
                "-f", "hls"
            ]
        }]
    }

    response = api.post("process", json=process_config)
    assert response.status_code in [200, 201], f"Failed to create process: {response.text}"

    created_id = response.json().get("id")
    assert created_id == process_id, f"Process ID mismatch: {created_id} != {process_id}"

    print(f"✅ Created test process: {process_id}")

    yield process_id

    # Cleanup
    print(f"🗑️  Cleaning up test process: {process_id}")
    api.delete(f"process/{process_id}")


# Test Suite

class TestAuthentication:
    """Test authentication functionality."""

    def test_login_success(self, api):
        """Test successful login."""
        assert api.access_token is not None
        assert len(api.access_token) > 0

    def test_authenticated_request(self, api):
        """Test making authenticated requests."""
        response = api.get("process")
        assert response.status_code == 200


class TestProcessManagement:
    """Test basic process management."""

    def test_list_processes(self, api):
        """Test listing processes."""
        response = api.get("process")
        assert response.status_code == 200
        processes = response.json()
        assert isinstance(processes, list)
        print(f"   Found {len(processes)} processes")

    def test_create_process(self, test_process):
        """Test process creation (via fixture)."""
        assert test_process is not None

    def test_get_process_state(self, api, test_process):
        """Test getting process state."""
        response = api.get(f"process/{test_process}/state")
        assert response.status_code == 200
        state = response.json()
        assert "order" in state
        print(f"   Process state: {state.get('order')}")


class TestMetadataStorage:
    """Test Feature #7: Metadata Storage."""

    def test_set_metadata(self, api, test_process):
        """Test setting process metadata."""
        metadata_value = "Test notes for automated testing"

        response = api.put(
            f"process/{test_process}/metadata/notes",
            json=metadata_value
        )
        assert response.status_code in [200, 204]
        print("   ✓ Set metadata 'notes'")

    def test_get_metadata(self, api, test_process):
        """Test retrieving process metadata."""
        response = api.get(f"process/{test_process}/metadata/notes")
        assert response.status_code == 200

        value = response.json()
        assert "Test notes for automated testing" in value
        print(f"   ✓ Retrieved metadata: {value}")

    def test_set_multiple_metadata(self, api, test_process):
        """Test setting multiple metadata fields."""
        # Set tags
        api.put(
            f"process/{test_process}/metadata/tags",
            json="test,automated,ci"
        )

        # Set description
        api.put(
            f"process/{test_process}/metadata/description",
            json="Automated test process"
        )

        # Verify both were set
        tags_response = api.get(f"process/{test_process}/metadata/tags")
        assert tags_response.status_code == 200
        assert "test,automated,ci" in tags_response.json()

        desc_response = api.get(f"process/{test_process}/metadata/description")
        assert desc_response.status_code == 200
        assert "Automated test process" in desc_response.json()

        print("   ✓ Set and verified multiple metadata fields")


class TestFileSystemBrowser:
    """Test Feature #8: File System Browser."""

    def test_list_filesystems(self, api):
        """Test listing available filesystems."""
        response = api.get("fs")
        assert response.status_code == 200

        filesystems = response.json()
        assert isinstance(filesystems, list)
        assert len(filesystems) > 0

        print(f"   Found {len(filesystems)} filesystems")

    def test_list_files(self, api):
        """Test listing files in filesystem."""
        # Get first filesystem
        fs_response = api.get("fs")
        filesystems = fs_response.json()
        first_fs = filesystems[0].get("name", "disk") if isinstance(filesystems[0], dict) else filesystems[0]

        # List files
        response = api.get(f"fs/{first_fs}")
        assert response.status_code == 200

        files = response.json()
        assert isinstance(files, list)
        print(f"   Found {len(files)} files in '{first_fs}'")

    def test_file_upload_download_delete(self, api):
        """Test file upload, download, and deletion."""
        test_content = b"Test file content for automated testing"
        test_filename = "test_upload.txt"

        # Upload file
        headers = {
            "Authorization": f"Bearer {api.access_token}",
            "Content-Type": "application/octet-stream"
        }

        upload_response = requests.put(
            f"{api.api_base}/fs/disk/{test_filename}",
            headers=headers,
            data=test_content,
            timeout=api.timeout
        )
        assert upload_response.status_code in [200, 201, 204]
        print(f"   ✓ Uploaded file: {test_filename}")

        # Download file
        download_response = requests.get(
            f"{api.api_base}/fs/disk/{test_filename}",
            headers=headers,
            timeout=api.timeout
        )
        assert download_response.status_code == 200
        assert download_response.content == test_content
        print(f"   ✓ Downloaded file matches uploaded content")

        # Delete file
        delete_response = requests.delete(
            f"{api.api_base}/fs/disk/{test_filename}",
            headers=headers,
            timeout=api.timeout
        )
        assert delete_response.status_code in [200, 204]
        print(f"   ✓ Deleted file: {test_filename}")


class TestDynamicOutputs:
    """Test Feature #9: Dynamic Output Management."""

    def test_list_outputs(self, api, test_process):
        """Test listing process outputs."""
        response = api.get(f"process/{test_process}")
        assert response.status_code == 200

        process_data = response.json()
        assert "output" in process_data
        outputs = process_data["output"]
        assert isinstance(outputs, list)

        initial_count = len(outputs)
        print(f"   Process has {initial_count} outputs")

        return initial_count

    def test_add_output(self, api, test_process):
        """Test adding output dynamically."""
        # Get initial count
        initial_response = api.get(f"process/{test_process}")
        initial_count = len(initial_response.json()["output"])

        # Add new output
        new_output = {
            "id": "output_test_1",
            "address": "http://localhost:8888/test2.m3u8",
            "options": [
                "-codec:v", "libx264",
                "-preset", "ultrafast",
                "-b:v", "800k",
                "-f", "hls"
            ]
        }

        response = api.post(f"process/{test_process}/outputs", json=new_output)
        assert response.status_code in [200, 201]
        print("   ✓ Added new output dynamically")

        # Verify output was added
        verify_response = api.get(f"process/{test_process}")
        new_count = len(verify_response.json()["output"])
        assert new_count > initial_count
        print(f"   ✓ Output count increased from {initial_count} to {new_count}")

    def test_remove_output(self, api, test_process):
        """Test removing output dynamically."""
        # Remove the test output we added
        response = api.delete(f"process/{test_process}/outputs/output_test_1")
        assert response.status_code in [200, 204]
        print("   ✓ Removed test output")


class TestPlayoutManagement:
    """Test Feature #10: Playout Management."""

    def test_get_playout_status(self, api, test_process):
        """Test getting playout status."""
        response = api.get(f"process/{test_process}/playout/input_0/status")
        assert response.status_code == 200

        status = response.json()
        assert "state" in status
        print(f"   Playout state: {status['state']}")

    def test_reopen_input(self, api, test_process):
        """Test reopening input connection."""
        response = api.put(f"process/{test_process}/playout/input_0/reopen")
        assert response.status_code in [200, 204]
        print("   ✓ Reopened input connection")


# Pytest configuration
def pytest_configure(config):
    """Configure pytest."""
    config.addinivalue_line(
        "markers", "integration: mark test as integration test"
    )


if __name__ == "__main__":
    """Allow running tests directly."""
    pytest.main([__file__, "-v", "-s"])
