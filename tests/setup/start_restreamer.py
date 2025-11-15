#!/usr/bin/env python3
"""
Cross-platform script to start and manage Restreamer Docker container for E2E tests.

This script:
1. Starts Restreamer using docker-compose
2. Waits for health checks to pass
3. Retrieves JWT authentication token
4. Provides utilities to stop and clean up the container
"""

import os
import sys
import time
import subprocess
import requests
from pathlib import Path
from typing import Optional, Dict, Tuple

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from utils.platform_helpers import (
    get_docker_compose_command,
    is_docker_available,
    load_test_config,
    platform_info
)


class RestreamerManager:
    """Manage Restreamer Docker container lifecycle for testing."""

    def __init__(self, compose_file: Optional[Path] = None):
        """
        Initialize Restreamer manager.

        Args:
            compose_file: Path to docker-compose file (default: docker-compose.e2e.yml in project root)
        """
        if compose_file is None:
            # Find project root (2 levels up from tests/setup/)
            project_root = Path(__file__).parent.parent.parent
            compose_file = project_root / "docker-compose.e2e.yml"

        self.compose_file = compose_file.resolve()
        if not self.compose_file.exists():
            raise FileNotFoundError(f"Docker Compose file not found: {self.compose_file}")

        self.config = load_test_config()
        self.base_url = self.config["api"]["base_url"]
        self.username = self.config["credentials"]["username"]
        self.password = self.config["credentials"]["password"]
        self.max_wait_time = self.config["docker"]["max_wait_time"]
        self.health_check_interval = self.config["docker"]["health_check_interval"]

        # Get docker-compose command
        self.compose_cmd = get_docker_compose_command()

    def _run_compose(self, *args: str, check: bool = True) -> subprocess.CompletedProcess:
        """
        Run docker-compose command.

        Args:
            *args: Arguments to pass to docker-compose
            check: Whether to check return code

        Returns:
            CompletedProcess result
        """
        cmd = [*self.compose_cmd, "-f", str(self.compose_file), *args]
        print(f"Running: {' '.join(cmd)}")
        return subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=check
        )

    def is_running(self) -> bool:
        """
        Check if Restreamer container is running.

        Returns:
            True if running, False otherwise
        """
        result = self._run_compose("ps", "-q", "restreamer", check=False)
        return bool(result.stdout.strip())

    def start(self, pull: bool = True) -> bool:
        """
        Start Restreamer container.

        Args:
            pull: Whether to pull latest image before starting

        Returns:
            True if successful, False otherwise
        """
        print("🚀 Starting Restreamer container...")

        # Check Docker availability
        if not is_docker_available():
            print("❌ Docker is not available or not running")
            return False

        # Pull latest image if requested
        if pull:
            print("📦 Pulling latest Restreamer image...")
            result = self._run_compose("pull", "restreamer", check=False)
            if result.returncode != 0:
                print(f"⚠️  Failed to pull image: {result.stderr}")
                print("Continuing with existing image...")

        # Start container
        print("🏃 Starting container...")
        result = self._run_compose("up", "-d", "restreamer", check=False)
        if result.returncode != 0:
            print(f"❌ Failed to start container: {result.stderr}")
            return False

        print("✓ Container started")
        return True

    def wait_for_health(self, timeout: Optional[int] = None) -> bool:
        """
        Wait for Restreamer to become healthy.

        Args:
            timeout: Maximum time to wait in seconds (default: from config)

        Returns:
            True if healthy, False if timeout
        """
        if timeout is None:
            timeout = self.max_wait_time

        print(f"⏳ Waiting for Restreamer to become healthy (timeout: {timeout}s)...")

        about_url = f"{self.base_url}/api/v3/about"
        start_time = time.time()
        attempts = 0

        while time.time() - start_time < timeout:
            attempts += 1
            try:
                response = requests.get(about_url, timeout=5)
                if response.status_code == 200:
                    elapsed = time.time() - start_time
                    print(f"✅ Restreamer is healthy! (after {elapsed:.1f}s, {attempts} attempts)")

                    # Print version info
                    try:
                        data = response.json()
                        print(f"   Version: {data.get('version', 'unknown')}")
                        print(f"   Name: {data.get('name', 'Restreamer')}")
                    except Exception:
                        pass

                    return True
            except requests.exceptions.RequestException:
                pass

            time.sleep(self.health_check_interval)
            if attempts % 5 == 0:
                print(f"   Still waiting... ({attempts} attempts)")

        print(f"❌ Timeout waiting for Restreamer to become healthy")
        return False

    def get_jwt_token(self) -> Optional[str]:
        """
        Retrieve JWT authentication token from Restreamer.

        Returns:
            JWT token string, or None if failed
        """
        print("🔑 Retrieving JWT token...")

        login_url = f"{self.base_url}/api/login"

        try:
            response = requests.post(
                login_url,
                json={
                    "username": self.username,
                    "password": self.password
                },
                timeout=10
            )

            if response.status_code == 200:
                data = response.json()
                token = data.get("access_token")
                if token:
                    print(f"✓ JWT token retrieved (length: {len(token)})")
                    return token
                else:
                    print("❌ No access_token in response")
                    return None
            else:
                print(f"❌ Login failed: {response.status_code}")
                print(f"   Response: {response.text}")
                return None

        except requests.exceptions.RequestException as e:
            print(f"❌ Failed to retrieve token: {e}")
            return None

    def stop(self) -> bool:
        """
        Stop Restreamer container.

        Returns:
            True if successful, False otherwise
        """
        print("🛑 Stopping Restreamer container...")
        result = self._run_compose("stop", "restreamer", check=False)
        if result.returncode == 0:
            print("✓ Container stopped")
            return True
        else:
            print(f"❌ Failed to stop container: {result.stderr}")
            return False

    def down(self, volumes: bool = False) -> bool:
        """
        Stop and remove Restreamer container.

        Args:
            volumes: Whether to remove volumes as well

        Returns:
            True if successful, False otherwise
        """
        print("🗑️  Removing Restreamer container...")
        args = ["down"]
        if volumes:
            args.append("-v")
            print("   (including volumes)")

        result = self._run_compose(*args, check=False)
        if result.returncode == 0:
            print("✓ Container removed")
            return True
        else:
            print(f"❌ Failed to remove container: {result.stderr}")
            return False

    def logs(self, follow: bool = False, tail: int = 100) -> None:
        """
        Show Restreamer container logs.

        Args:
            follow: Whether to follow logs
            tail: Number of lines to show
        """
        args = ["logs"]
        if follow:
            args.append("-f")
        args.extend(["--tail", str(tail), "restreamer"])

        self._run_compose(*args, check=False)

    def get_status(self) -> Dict[str, any]:
        """
        Get comprehensive status of Restreamer container.

        Returns:
            Dictionary with status information
        """
        status = {
            "running": self.is_running(),
            "healthy": False,
            "authenticated": False,
            "version": None,
            "token": None
        }

        if status["running"]:
            # Check health
            try:
                response = requests.get(f"{self.base_url}/api/v3/about", timeout=5)
                if response.status_code == 200:
                    status["healthy"] = True
                    data = response.json()
                    status["version"] = data.get("version")
            except Exception:
                pass

            # Try to get token
            if status["healthy"]:
                token = self.get_jwt_token()
                if token:
                    status["authenticated"] = True
                    status["token"] = token

        return status


def main():
    """Main entry point for CLI usage."""
    import argparse

    parser = argparse.ArgumentParser(description="Manage Restreamer Docker container for E2E tests")
    parser.add_argument("action", choices=["start", "stop", "down", "status", "logs", "restart"],
                        help="Action to perform")
    parser.add_argument("--no-pull", action="store_true",
                        help="Don't pull latest image when starting")
    parser.add_argument("--volumes", action="store_true",
                        help="Remove volumes when using 'down'")
    parser.add_argument("--follow", "-f", action="store_true",
                        help="Follow logs (for 'logs' action)")
    parser.add_argument("--tail", type=int, default=100,
                        help="Number of log lines to show (default: 100)")
    parser.add_argument("--compose-file", type=Path,
                        help="Path to docker-compose file")

    args = parser.parse_args()

    try:
        manager = RestreamerManager(compose_file=args.compose_file)

        if args.action == "start":
            if manager.start(pull=not args.no_pull):
                if manager.wait_for_health():
                    token = manager.get_jwt_token()
                    if token:
                        print(f"\n✅ Restreamer is ready!")
                        print(f"   URL: {manager.base_url}")
                        print(f"   Username: {manager.username}")
                        print(f"   Token: {token[:20]}...")
                        sys.exit(0)
                    else:
                        print("\n⚠️  Restreamer is running but authentication failed")
                        sys.exit(1)
                else:
                    print("\n❌ Restreamer failed health check")
                    sys.exit(1)
            else:
                print("\n❌ Failed to start Restreamer")
                sys.exit(1)

        elif args.action == "stop":
            sys.exit(0 if manager.stop() else 1)

        elif args.action == "down":
            sys.exit(0 if manager.down(volumes=args.volumes) else 1)

        elif args.action == "restart":
            manager.stop()
            time.sleep(2)
            if manager.start(pull=not args.no_pull):
                if manager.wait_for_health():
                    sys.exit(0)
            sys.exit(1)

        elif args.action == "status":
            status = manager.get_status()
            print(f"\n📊 Restreamer Status:")
            print(f"   Running: {'✅' if status['running'] else '❌'}")
            print(f"   Healthy: {'✅' if status['healthy'] else '❌'}")
            print(f"   Authenticated: {'✅' if status['authenticated'] else '❌'}")
            if status['version']:
                print(f"   Version: {status['version']}")
            sys.exit(0 if status['running'] and status['healthy'] else 1)

        elif args.action == "logs":
            manager.logs(follow=args.follow, tail=args.tail)
            sys.exit(0)

    except Exception as e:
        print(f"❌ Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
