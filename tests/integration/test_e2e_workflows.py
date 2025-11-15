"""
End-to-End Workflow Tests

Tests complete real-world workflows across multiple features:
1. Content Creator Workflow - Stream creation, metadata, asset upload
2. Multi-Platform Streaming - Simultaneous streaming to multiple platforms
3. Recording Management - Upload, tag, download, archive
4. Team Collaboration - Shared notes, asset management, status tracking
"""

import os
import sys
import pytest
import requests
import tempfile
from datetime import datetime
from pathlib import Path
from typing import Optional, Dict

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
    """Get valid access token for workflows."""
    response = requests.post(
        f"{base_url}/api/login",
        json={
            "username": credentials["username"],
            "password": credentials["password"]
        },
        timeout=timeout
    )
    assert response.status_code == 200, "Failed to authenticate"
    token = response.json()["access_token"]
    print(f"✅ Authenticated successfully")
    return token


class WorkflowHelper:
    """Helper class for workflow operations."""

    def __init__(self, api_base: str, access_token: str, timeout: int):
        self.api_base = api_base
        self.access_token = access_token
        self.timeout = timeout
        self.headers = {
            "Authorization": f"Bearer {access_token}",
            "Content-Type": "application/json"
        }

    def create_process(self, process_id: str, reference: str = "test") -> bool:
        """Create a test process."""
        response = requests.post(
            f"{self.api_base}/process",
            headers=self.headers,
            json={"id": process_id, "reference": reference},
            timeout=self.timeout
        )
        return response.status_code in [200, 201]

    def delete_process(self, process_id: str):
        """Delete a process."""
        try:
            requests.delete(
                f"{self.api_base}/process/{process_id}",
                headers=self.headers,
                timeout=self.timeout
            )
        except Exception:
            pass

    def set_metadata(self, process_id: str, key: str, value: str) -> bool:
        """Set process metadata."""
        response = requests.put(
            f"{self.api_base}/process/{process_id}/metadata/{key}",
            headers=self.headers,
            json=value,
            timeout=self.timeout
        )
        return response.status_code in [200, 204]

    def get_metadata(self, process_id: str, key: str) -> Optional[str]:
        """Get process metadata."""
        response = requests.get(
            f"{self.api_base}/process/{process_id}/metadata/{key}",
            headers=self.headers,
            timeout=self.timeout
        )
        if response.status_code == 200:
            return response.json()
        return None

    def upload_file(self, filesystem: str, path: str, content: bytes) -> bool:
        """Upload file to filesystem."""
        headers = {
            "Authorization": f"Bearer {self.access_token}",
            "Content-Type": "application/octet-stream"
        }
        response = requests.put(
            f"{self.api_base}/fs/{filesystem}/{path}",
            headers=headers,
            data=content,
            timeout=self.timeout
        )
        return response.status_code in [200, 201, 204]

    def download_file(self, filesystem: str, path: str) -> Optional[bytes]:
        """Download file from filesystem."""
        headers = {"Authorization": f"Bearer {self.access_token}"}
        response = requests.get(
            f"{self.api_base}/fs/{filesystem}/{path}",
            headers=headers,
            timeout=self.timeout
        )
        if response.status_code == 200:
            return response.content
        return None

    def delete_file(self, filesystem: str, path: str):
        """Delete file from filesystem."""
        try:
            headers = {"Authorization": f"Bearer {self.access_token}"}
            requests.delete(
                f"{self.api_base}/fs/{filesystem}/{path}",
                headers=headers,
                timeout=self.timeout
            )
        except Exception:
            pass

    def get_process_state(self, process_id: str) -> Optional[str]:
        """Get process state."""
        response = requests.get(
            f"{self.api_base}/process/{process_id}/state",
            headers=self.headers,
            timeout=self.timeout
        )
        if response.status_code == 200:
            return response.json().get("order")
        return None

    def list_files(self, filesystem: str = "disk") -> list:
        """List files in filesystem."""
        response = requests.get(
            f"{self.api_base}/fs/{filesystem}",
            headers=self.headers,
            timeout=self.timeout
        )
        if response.status_code == 200:
            return response.json()
        return []


@pytest.fixture
def helper(api_base, access_token, timeout):
    """Get workflow helper."""
    return WorkflowHelper(api_base, access_token, timeout)


# ========================================================================
# WORKFLOW 1: Content Creator Workflow
# ========================================================================

class TestContentCreatorWorkflow:
    """Test complete content creator workflow."""

    def test_content_creator_workflow(self, helper):
        """
        Workflow: Create Stream → Add Metadata → Upload Assets → Monitor

        This simulates a content creator setting up a live gaming stream.
        """
        process_id = f"content_creator_stream_{os.getpid()}"
        print(f"\n{'='*70}")
        print(f"WORKFLOW: Content Creator - Create Stream and Upload Assets")
        print(f"{'='*70}")

        try:
            # Step 1: Create streaming process
            print("📝 Step 1/5: Creating streaming process...")
            assert helper.create_process(process_id, "content_creator_workflow"), \
                "Failed to create streaming process"
            print(f"   ✓ Process created: {process_id}")

            # Step 2: Add descriptive metadata
            print("📝 Step 2/5: Adding stream metadata (title, description, tags)...")

            today = datetime.now().strftime("%Y-%m-%d")
            assert helper.set_metadata(
                process_id, "title",
                f"My Live Gaming Stream - {today}"
            ), "Failed to set title"

            assert helper.set_metadata(
                process_id, "description",
                "Playing Minecraft with viewers! Drop by and say hi!"
            ), "Failed to set description"

            assert helper.set_metadata(
                process_id, "tags",
                "gaming,minecraft,live,interactive"
            ), "Failed to set tags"

            # Verify metadata
            title = helper.get_metadata(process_id, "title")
            assert title is not None and "Gaming Stream" in title, \
                "Failed to verify title"
            print("   ✓ Metadata added and verified")

            # Step 3: Upload stream assets
            print("📝 Step 3/5: Uploading stream assets (thumbnail, overlay)...")

            thumbnail_data = f"Mock Thumbnail Data {datetime.now()}".encode()
            assert helper.upload_file(
                "disk",
                f"stream_assets/thumbnail_{process_id}.png",
                thumbnail_data
            ), "Failed to upload thumbnail"

            overlay_data = f"Mock Overlay Data {datetime.now()}".encode()
            assert helper.upload_file(
                "disk",
                f"stream_assets/overlay_{process_id}.png",
                overlay_data
            ), "Failed to upload overlay"
            print("   ✓ Stream assets uploaded")

            # Step 4: Verify uploaded files
            print("📝 Step 4/5: Verifying uploaded assets...")

            files = helper.list_files("disk")
            thumbnail_found = any(
                f.get("name", "").startswith("thumbnail_") if isinstance(f, dict) else False
                for f in files
            )
            print(f"   {'✓' if thumbnail_found else '⚠'} Thumbnail verification")

            # Step 5: Monitor process state
            print("📝 Step 5/5: Checking stream status...")

            state = helper.get_process_state(process_id)
            assert state is not None, "Failed to get process state"
            print(f"   ✓ Stream state: {state}")

            print(f"\n✅ Content Creator Workflow PASSED")

        finally:
            # Cleanup
            print("🗑️  Cleaning up workflow resources...")
            helper.delete_process(process_id)
            helper.delete_file("disk", f"stream_assets/thumbnail_{process_id}.png")
            helper.delete_file("disk", f"stream_assets/overlay_{process_id}.png")


# ========================================================================
# WORKFLOW 2: Multi-Platform Streaming
# ========================================================================

class TestMultiPlatformWorkflow:
    """Test multi-platform streaming workflow."""

    def test_multi_platform_workflow(self, helper):
        """
        Workflow: Configure streaming to YouTube, Twitch, Facebook simultaneously

        This simulates setting up multi-platform streaming with platform-specific configs.
        """
        process_id = f"multistream_{os.getpid()}"
        print(f"\n{'='*70}")
        print(f"WORKFLOW: Multi-Platform - Stream to Multiple Platforms")
        print(f"{'='*70}")

        try:
            # Step 1: Create base process
            print("📝 Step 1/4: Creating multi-platform streaming process...")
            assert helper.create_process(process_id, "multistream_workflow"), \
                "Failed to create process"
            print("   ✓ Base process created")

            # Step 2: Add platform-specific metadata
            print("📝 Step 2/4: Configuring platform-specific settings...")

            import json
            platforms_config = {
                "youtube": {"stream_key": "yt_key_12345", "resolution": "1080p"},
                "twitch": {"stream_key": "twitch_key_67890", "resolution": "1080p"},
                "facebook": {"stream_key": "fb_key_abcde", "resolution": "720p"}
            }

            assert helper.set_metadata(
                process_id, "platforms",
                json.dumps(platforms_config)
            ), "Failed to set platform metadata"
            print("   ✓ Platform metadata configured")

            # Step 3: Record stream configuration
            print("📝 Step 3/4: Storing stream configuration...")

            config_note = f"Multi-platform stream configured on {datetime.now()}"
            assert helper.set_metadata(process_id, "notes", config_note), \
                "Failed to set configuration notes"
            print("   ✓ Configuration recorded")

            # Step 4: Verify all metadata
            print("📝 Step 4/4: Verifying multi-platform configuration...")

            platforms_verify = helper.get_metadata(process_id, "platforms")
            assert platforms_verify is not None and "youtube" in platforms_verify, \
                "Failed to verify platform configuration"
            print("   ✓ Multi-platform configuration verified")

            print(f"\n✅ Multi-Platform Streaming Workflow PASSED")

        finally:
            # Cleanup
            print("🗑️  Cleaning up workflow resources...")
            helper.delete_process(process_id)


# ========================================================================
# WORKFLOW 3: Recording Management
# ========================================================================

class TestRecordingManagementWorkflow:
    """Test recording management workflow."""

    def test_recording_management_workflow(self, helper):
        """
        Workflow: Upload Recording → Tag → Download → Archive

        This simulates managing stream recordings with metadata and archiving.
        """
        print(f"\n{'='*70}")
        print(f"WORKFLOW: Recording Management - Upload, Tag, Archive")
        print(f"{'='*70}")

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        recording_name = f"recording_{timestamp}.mp4"
        catalog_id = f"catalog_recording_{timestamp}"

        try:
            # Step 1: Upload recording
            print("📝 Step 1/5: Uploading stream recording...")

            recording_data = f"Mock Recording Data - {datetime.now()}".encode()
            assert helper.upload_file(
                "disk",
                f"recordings/{recording_name}",
                recording_data
            ), "Failed to upload recording"
            print(f"   ✓ Recording uploaded: {recording_name}")

            # Step 2: Create catalog entry with metadata
            print("📝 Step 2/5: Creating catalog entry with tags...")

            assert helper.create_process(catalog_id, "recording_catalog"), \
                "Failed to create catalog entry"

            # Tag the recording
            assert helper.set_metadata(catalog_id, "tags", "archived,gaming,highlight"), \
                "Failed to set tags"

            today = datetime.now().strftime("%Y-%m-%d")
            assert helper.set_metadata(
                catalog_id, "title",
                f"Epic Stream Moments - {today}"
            ), "Failed to set title"

            assert helper.set_metadata(
                catalog_id, "file_path",
                f"/recordings/{recording_name}"
            ), "Failed to set file path"

            print("   ✓ Catalog entry created with tags")

            # Step 3: List and verify recordings
            print("📝 Step 3/5: Verifying recording in catalog...")

            files = helper.list_files("disk")
            recording_found = any(
                f.get("name") == recording_name if isinstance(f, dict)
                else f == recording_name
                for f in files
            )
            if recording_found:
                print("   ✓ Recording found in filesystem")
            else:
                print("   ⚠ Recording not found in listing")

            # Step 4: Download for backup
            print("📝 Step 4/5: Downloading recording for backup...")

            downloaded = helper.download_file("disk", f"recordings/{recording_name}")
            assert downloaded is not None, "Failed to download recording"
            assert downloaded == recording_data, "Downloaded data doesn't match uploaded"
            print("   ✓ Recording downloaded successfully")

            # Step 5: Archive (mark as archived in metadata)
            print("📝 Step 5/5: Marking as archived...")

            archive_date = f"Archived on {datetime.now()}"
            assert helper.set_metadata(catalog_id, "archived", archive_date), \
                "Failed to set archive metadata"
            print("   ✓ Recording marked as archived")

            print(f"\n✅ Recording Management Workflow PASSED")

        finally:
            # Cleanup
            print("🗑️  Cleaning up workflow resources...")
            helper.delete_process(catalog_id)
            helper.delete_file("disk", f"recordings/{recording_name}")


# ========================================================================
# WORKFLOW 4: Team Collaboration
# ========================================================================

class TestTeamCollaborationWorkflow:
    """Test team collaboration workflow."""

    def test_team_collaboration_workflow(self, helper):
        """
        Workflow: Shared Notes → Asset Management → Version Control

        This simulates a production team collaborating on a project.
        """
        project_id = f"team_project_{os.getpid()}"
        print(f"\n{'='*70}")
        print(f"WORKFLOW: Team Collaboration - Shared Project Management")
        print(f"{'='*70}")

        try:
            # Step 1: Create project
            print("📝 Step 1/4: Creating team project...")

            assert helper.create_process(project_id, "team_collaboration"), \
                "Failed to create team project"
            print("   ✓ Team project created")

            # Step 2: Team members add notes
            print("📝 Step 2/4: Team members contributing notes...")

            assert helper.set_metadata(
                project_id, "director_notes",
                "Director: Need more B-roll footage of city skyline"
            ), "Failed to add director notes"

            assert helper.set_metadata(
                project_id, "editor_notes",
                "Editor: Color grading complete, ready for review"
            ), "Failed to add editor notes"

            assert helper.set_metadata(
                project_id, "producer_notes",
                "Producer: Budget approved for additional filming day"
            ), "Failed to add producer notes"

            print("   ✓ Team notes added (director, editor, producer)")

            # Step 3: Upload shared assets
            print("📝 Step 3/4: Uploading shared project assets...")

            for asset in ["script", "storyboard", "timeline"]:
                content = f"Project {asset} v1.0 - {datetime.now()}".encode()
                assert helper.upload_file(
                    "disk",
                    f"team_assets/{project_id}_{asset}.txt",
                    content
                ), f"Failed to upload {asset}"

            print("   ✓ Project assets uploaded (script, storyboard, timeline)")

            # Step 4: Track project status
            print("📝 Step 4/4: Updating project status...")

            import json
            status = {
                "phase": "post-production",
                "progress": 75,
                "deadline": "2025-12-31",
                "team": ["director", "editor", "producer"]
            }

            assert helper.set_metadata(
                project_id, "status",
                json.dumps(status)
            ), "Failed to set project status"

            # Verify status
            status_verify = helper.get_metadata(project_id, "status")
            assert status_verify is not None and "post-production" in status_verify, \
                "Failed to verify project status"
            print("   ✓ Project status tracked and verified")

            print(f"\n✅ Team Collaboration Workflow PASSED")

        finally:
            # Cleanup
            print("🗑️  Cleaning up workflow resources...")
            helper.delete_process(project_id)
            for asset in ["script", "storyboard", "timeline"]:
                helper.delete_file("disk", f"team_assets/{project_id}_{asset}.txt")


# Pytest configuration
def pytest_configure(config):
    """Configure pytest."""
    config.addinivalue_line(
        "markers", "workflow: mark test as end-to-end workflow test"
    )


if __name__ == "__main__":
    """Allow running tests directly."""
    pytest.main([__file__, "-v", "-s"])
