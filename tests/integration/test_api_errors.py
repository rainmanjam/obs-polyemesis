"""
API Error Handling and Edge Case Tests

Tests error responses, invalid inputs, edge cases, and stress scenarios.
"""

import os
import sys
import pytest
import requests
import concurrent.futures
from pathlib import Path
from typing import Optional

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from utils.platform_helpers import load_test_config


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
def api_base(base_url, config):
    """Get full API base URL."""
    api_version = config["api"]["version"]
    return f"{base_url}/api/{api_version}"


@pytest.fixture(scope="module")
def credentials(config):
    """Get authentication credentials."""
    return config["credentials"]


@pytest.fixture(scope="module")
def timeout(config):
    """Get API timeout."""
    return config["api"]["timeout"]


@pytest.fixture(scope="module")
def access_token(base_url, credentials, timeout):
    """Get valid access token for tests."""
    response = requests.post(
        f"{base_url}/api/login",
        json={
            "username": credentials["username"],
            "password": credentials["password"]
        },
        timeout=timeout
    )
    assert response.status_code == 200, "Failed to authenticate"
    return response.json()["access_token"]


def create_test_process(api_base: str, access_token: str, process_id: str, timeout: int):
    """
    Helper to create a test process.

    Args:
        api_base: Base API URL
        access_token: JWT token
        process_id: ID for the process
        timeout: Request timeout

    Returns:
        Process ID if successful, None otherwise
    """
    headers = {
        "Authorization": f"Bearer {access_token}",
        "Content-Type": "application/json"
    }

    process_data = {
        "id": process_id,
        "reference": "test"
    }

    try:
        response = requests.post(
            f"{api_base}/process",
            headers=headers,
            json=process_data,
            timeout=timeout
        )

        if response.status_code in [200, 201]:
            return response.json().get("id")
    except Exception:
        pass

    return None


def cleanup_test_process(api_base: str, access_token: str, process_id: str, timeout: int):
    """Helper to cleanup a test process."""
    headers = {"Authorization": f"Bearer {access_token}"}
    try:
        requests.delete(
            f"{api_base}/process/{process_id}",
            headers=headers,
            timeout=timeout
        )
    except Exception:
        pass


# ========================================================================
# ERROR HANDLING TESTS
# ========================================================================

class TestAuthenticationErrors:
    """Test authentication error handling."""

    def test_invalid_credentials(self, base_url, timeout):
        """Test rejection of invalid credentials."""
        response = requests.post(
            f"{base_url}/api/login",
            json={
                "username": "invalid",
                "password": "wrong"
            },
            timeout=timeout
        )

        assert response.status_code == 401, \
            f"Expected HTTP 401, got {response.status_code}"
        print("   ✓ Correctly rejected invalid credentials (HTTP 401)")

    def test_missing_auth_token(self, api_base, timeout):
        """Test rejection of requests without auth token."""
        response = requests.get(
            f"{api_base}/process",
            timeout=timeout
        )

        assert response.status_code == 401, \
            f"Expected HTTP 401, got {response.status_code}"
        print("   ✓ Correctly rejected request without auth token (HTTP 401)")

    def test_invalid_auth_token(self, api_base, timeout):
        """Test rejection of invalid auth tokens."""
        headers = {"Authorization": "Bearer invalid_token_12345"}

        response = requests.get(
            f"{api_base}/process",
            headers=headers,
            timeout=timeout
        )

        assert response.status_code == 401, \
            f"Expected HTTP 401, got {response.status_code}"
        print("   ✓ Correctly rejected invalid auth token (HTTP 401)")


class TestResourceErrors:
    """Test error handling for nonexistent resources."""

    def test_nonexistent_process(self, api_base, access_token, timeout):
        """Test 404 for nonexistent process."""
        headers = {"Authorization": f"Bearer {access_token}"}

        response = requests.get(
            f"{api_base}/process/nonexistent_process_999",
            headers=headers,
            timeout=timeout
        )

        assert response.status_code == 404, \
            f"Expected HTTP 404, got {response.status_code}"
        print("   ✓ Correctly returned 404 for nonexistent process")

    def test_nonexistent_filesystem(self, api_base, access_token, timeout):
        """Test 404 for nonexistent filesystem."""
        headers = {"Authorization": f"Bearer {access_token}"}

        response = requests.get(
            f"{api_base}/fs/nonexistent_fs",
            headers=headers,
            timeout=timeout
        )

        assert response.status_code == 404, \
            f"Expected HTTP 404, got {response.status_code}"
        print("   ✓ Correctly returned 404 for nonexistent filesystem")

    def test_nonexistent_file(self, api_base, access_token, timeout):
        """Test 404 for nonexistent file download."""
        headers = {"Authorization": f"Bearer {access_token}"}

        response = requests.get(
            f"{api_base}/fs/disk/nonexistent_file_999.txt",
            headers=headers,
            timeout=timeout
        )

        assert response.status_code == 404, \
            f"Expected HTTP 404, got {response.status_code}"
        print("   ✓ Correctly returned 404 for nonexistent file")


class TestInvalidPayloads:
    """Test handling of invalid request payloads."""

    def test_invalid_json(self, api_base, access_token, timeout):
        """Test rejection of invalid JSON."""
        headers = {
            "Authorization": f"Bearer {access_token}",
            "Content-Type": "application/json"
        }

        response = requests.post(
            f"{api_base}/process",
            headers=headers,
            data='{invalid json syntax}',
            timeout=timeout
        )

        assert response.status_code == 400, \
            f"Expected HTTP 400, got {response.status_code}"
        print("   ✓ Correctly rejected invalid JSON (HTTP 400)")


# ========================================================================
# EDGE CASE TESTS
# ========================================================================

class TestMetadataEdgeCases:
    """Test edge cases for metadata storage."""

    def test_empty_metadata_value(self, api_base, access_token, timeout):
        """Test setting empty metadata value."""
        process_id = f"test_empty_meta_{os.getpid()}"

        try:
            # Create test process
            pid = create_test_process(api_base, access_token, process_id, timeout)
            assert pid is not None, "Failed to create test process"

            headers = {
                "Authorization": f"Bearer {access_token}",
                "Content-Type": "application/json"
            }

            # Set empty metadata
            response = requests.put(
                f"{api_base}/process/{process_id}/metadata/empty_test",
                headers=headers,
                json="",
                timeout=timeout
            )

            assert response.status_code == 200, \
                f"Failed to handle empty metadata (HTTP {response.status_code})"
            print("   ✓ Successfully handled empty metadata value")

        finally:
            cleanup_test_process(api_base, access_token, process_id, timeout)

    def test_special_characters_in_metadata(self, api_base, access_token, timeout):
        """Test metadata with special characters."""
        process_id = f"test_special_{os.getpid()}"

        try:
            # Create test process
            pid = create_test_process(api_base, access_token, process_id, timeout)
            assert pid is not None, "Failed to create test process"

            headers = {
                "Authorization": f"Bearer {access_token}",
                "Content-Type": "application/json"
            }

            # Special characters: quotes, newlines, unicode
            special_value = 'Test with "quotes", \'apostrophes\', newlines\nand unicode: 你好世界 🎬'

            response = requests.put(
                f"{api_base}/process/{process_id}/metadata/special_chars",
                headers=headers,
                json=special_value,
                timeout=timeout
            )

            assert response.status_code in [200, 204]

            # Verify we can retrieve it
            get_response = requests.get(
                f"{api_base}/process/{process_id}/metadata/special_chars",
                headers=headers,
                timeout=timeout
            )

            assert get_response.status_code == 200
            assert "quotes" in get_response.json()
            print("   ✓ Successfully handled special characters in metadata")

        finally:
            cleanup_test_process(api_base, access_token, process_id, timeout)

    def test_large_metadata_value(self, api_base, access_token, timeout):
        """Test large metadata value (10KB)."""
        process_id = f"test_large_meta_{os.getpid()}"

        try:
            # Create test process
            pid = create_test_process(api_base, access_token, process_id, timeout)
            assert pid is not None, "Failed to create test process"

            headers = {
                "Authorization": f"Bearer {access_token}",
                "Content-Type": "application/json"
            }

            # Generate 10KB of data
            import base64
            large_value = base64.b64encode(os.urandom(10000)).decode('ascii')

            response = requests.put(
                f"{api_base}/process/{process_id}/metadata/large_data",
                headers=headers,
                json=large_value,
                timeout=timeout
            )

            assert response.status_code == 200, \
                f"Failed to handle large metadata (HTTP {response.status_code})"
            print("   ✓ Successfully handled large metadata value (10KB)")

        finally:
            cleanup_test_process(api_base, access_token, process_id, timeout)

    def test_concurrent_metadata_updates(self, api_base, access_token, timeout):
        """Test concurrent metadata updates."""
        process_id = f"test_concurrent_{os.getpid()}"

        try:
            # Create test process
            pid = create_test_process(api_base, access_token, process_id, timeout)
            assert pid is not None, "Failed to create test process"

            headers = {
                "Authorization": f"Bearer {access_token}",
                "Content-Type": "application/json"
            }

            # Launch 5 concurrent metadata updates
            def update_metadata(index: int) -> bool:
                try:
                    response = requests.put(
                        f"{api_base}/process/{process_id}/metadata/concurrent_{index}",
                        headers=headers,
                        json=f"Concurrent update {index}",
                        timeout=timeout
                    )
                    return response.status_code in [200, 204]
                except Exception:
                    return False

            with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
                futures = [executor.submit(update_metadata, i) for i in range(1, 6)]
                results = [f.result() for f in concurrent.futures.as_completed(futures)]

            # Verify all updates succeeded
            success_count = 0
            for i in range(1, 6):
                response = requests.get(
                    f"{api_base}/process/{process_id}/metadata/concurrent_{i}",
                    headers=headers,
                    timeout=timeout
                )

                if response.status_code == 200 and "Concurrent update" in response.json():
                    success_count += 1

            assert success_count == 5, \
                f"Only {success_count}/5 concurrent updates succeeded"
            print(f"   ✓ All 5 concurrent metadata updates succeeded")

        finally:
            cleanup_test_process(api_base, access_token, process_id, timeout)


class TestFileSystemEdgeCases:
    """Test edge cases for file system operations."""

    def test_large_file_upload(self, api_base, access_token, timeout):
        """Test large file upload (10MB)."""
        headers = {
            "Authorization": f"Bearer {access_token}",
            "Content-Type": "application/octet-stream"
        }

        # Generate 10MB test data
        large_data = os.urandom(10 * 1024 * 1024)

        try:
            response = requests.put(
                f"{api_base}/fs/disk/large_test_file.bin",
                headers=headers,
                data=large_data,
                timeout=max(timeout, 60)  # Increase timeout for large upload
            )

            assert response.status_code in [200, 201, 204], \
                f"Failed to upload large file (HTTP {response.status_code})"
            print("   ✓ Successfully uploaded 10MB file")

        finally:
            # Cleanup
            requests.delete(
                f"{api_base}/fs/disk/large_test_file.bin",
                headers={"Authorization": f"Bearer {access_token}"},
                timeout=timeout
            )


# ========================================================================
# STRESS TESTS
# ========================================================================

class TestStressScenarios:
    """Test stress scenarios."""

    def test_rapid_api_requests(self, api_base, access_token, timeout):
        """Test rapid API requests (20 concurrent requests)."""
        headers = {"Authorization": f"Bearer {access_token}"}

        def make_request() -> bool:
            try:
                response = requests.get(
                    f"{api_base}/process",
                    headers=headers,
                    timeout=timeout
                )
                return response.status_code == 200
            except Exception:
                return False

        total_requests = 20

        with concurrent.futures.ThreadPoolExecutor(max_workers=20) as executor:
            futures = [executor.submit(make_request) for _ in range(total_requests)]
            results = [f.result() for f in concurrent.futures.as_completed(futures)]

        success_count = sum(results)

        assert success_count == total_requests, \
            f"Only {success_count}/{total_requests} rapid requests succeeded"
        print(f"   ✓ All {total_requests} rapid requests succeeded")


# Pytest configuration
def pytest_configure(config):
    """Configure pytest."""
    config.addinivalue_line(
        "markers", "error: mark test as error handling test"
    )
    config.addinivalue_line(
        "markers", "edge: mark test as edge case test"
    )
    config.addinivalue_line(
        "markers", "stress: mark test as stress test"
    )


if __name__ == "__main__":
    """Allow running tests directly."""
    pytest.main([__file__, "-v", "-s"])
