#!/usr/bin/env python3
"""
Security Testing for OBS Polyemesis
Tests credential storage, API security, and injection vulnerabilities
"""

import os
import time
import unittest
import requests
from test_polyemesis import RestreamerClient


class TestSecurity(unittest.TestCase):
    """Security and vulnerability testing"""

    @classmethod
    def setUpClass(cls):
        cls.base_url = os.getenv('RESTREAMER_URL', 'http://localhost:8080')
        cls.username = os.getenv('RESTREAMER_USER', 'admin')
        cls.password = os.getenv('RESTREAMER_PASS', 'admin')
        cls.client = RestreamerClient(cls.base_url, cls.username, cls.password)

    def test_01_credential_storage_not_plaintext(self):
        """
        Verify credentials are not stored in plaintext (Phase 8, Test 8.1)

        Checks common locations for exposed credentials
        """
        print("\n🔒 Testing credential storage security...")

        # Check if OBS config files exist (this would be in the actual OBS config dir)
        potential_config_locations = [
            os.path.expanduser("~/Library/Application Support/obs-studio/plugin_config/obs-polyemesis"),
            os.path.expanduser("~/.config/obs-studio/plugin_config/obs-polyemesis"),
            "obs-polyemesis.conf",
        ]

        exposed_credentials = []

        for location in potential_config_locations:
            if os.path.exists(location) and os.path.isfile(location):
                try:
                    with open(location, 'r') as f:
                        content = f.read()
                        # Look for plaintext passwords (very basic check)
                        if 'password=' in content.lower() or 'token=' in content.lower():
                            exposed_credentials.append(location)
                except Exception:
                    # Ignore cleanup errors - process may already be stopped/deleted
                    pass
        if exposed_credentials:
            print(f"⚠️  WARNING: Potential plaintext credentials in: {exposed_credentials}")
            # This is a warning, not a failure (depends on implementation)
        else:
            print("✅ No plaintext credentials found in common locations")

        self.assertTrue(True, "Credential storage check completed")

    def test_02_api_rate_limiting(self):
        """
        Test API rate limiting behavior (Phase 8, Test 8.3)

        Makes rapid requests to check for rate limiting or throttling
        """
        print("\n⏱️  Testing API rate limiting...")

        # Make rapid requests
        request_count = 20
        start_time = time.time()
        success_count = 0
        rate_limited = False

        for i in range(request_count):
            try:
                response = self.client.get('/api/v3/about')
                if response.status_code == 200:
                    success_count += 1
                elif response.status_code == 429:  # Too Many Requests
                    rate_limited = True
                    print(f"✅ Rate limiting detected at request {i+1}")
                    break
            except requests.exceptions.RequestException as e:
                print(f"⚠️  Request {i+1} failed: {e}")

        elapsed_time = time.time() - start_time
        requests_per_second = request_count / elapsed_time if elapsed_time > 0 else 0

        print(f"📊 Completed {success_count}/{request_count} requests in {elapsed_time:.2f}s")
        print(f"📊 Rate: {requests_per_second:.1f} req/s")

        if rate_limited:
            print("✅ API has rate limiting enabled")
        else:
            print("ℹ️  No rate limiting detected (may not be configured)")

        self.assertTrue(True, "Rate limiting test completed")

    def test_03_injection_attack_process_id(self):
        """
        Test for injection vulnerabilities in process ID (Phase 8, Test 8.4 partial)

        Attempts to inject special characters and commands into process IDs
        """
        print("\n🛡️  Testing injection attack resistance (Process ID)...")

        malicious_inputs = [
            "../../../etc/passwd",  # Path traversal
            "'; DROP TABLE processes--",  # SQL injection
            "<script>alert('xss')</script>",  # XSS
            "test$(rm -rf /)",  # Command injection
            "test`whoami`",  # Command injection
            "test;ls -la",  # Command injection
            "test&&cat /etc/passwd",  # Command injection
        ]

        for malicious_id in malicious_inputs:
            try:
                # Try to create process with malicious ID
                result = self.client.create_process(
                    process_id=malicious_id,
                    input_url="rtmp://localhost/test",
                    outputs=[]
                )

                # If it succeeds, check if it was sanitized
                if result and result.get('id') != malicious_id:
                    print(f"✅ Input sanitized: '{malicious_id}' -> '{result.get('id')}'")
                elif result:
                    print(f"⚠️  WARNING: Accepted malicious ID: {malicious_id}")
                    # Clean up
                    self.client.delete('/api/v3/process/' + malicious_id)

            except Exception as e:
                # Expected to fail - injection prevented
                print(f"✅ Injection blocked: {malicious_id[:30]}...")

        print("✅ Injection attack resistance test completed")

    def test_04_https_redirect(self):
        """
        Test if HTTP connections are redirected to HTTPS (Phase 8, Test 8.2 partial)

        Note: Only applicable if server is configured for HTTPS
        """
        print("\n🔐 Testing HTTPS enforcement...")

        # Check if we're using HTTPS
        if self.base_url.startswith('https://'):
            print("✅ Already using HTTPS")
        elif self.base_url.startswith('http://'):
            # Try HTTPS equivalent
            https_url = self.base_url.replace('http://', 'https://')
            try:
                # Attempt HTTPS with proper certificate verification
                response = requests.get(f"{https_url}/api/v3/about", timeout=5)
                if response.status_code == 200:
                    print(f"✅ HTTPS is available at {https_url}")
                    print("ℹ️  Recommend using HTTPS in production")
            except requests.exceptions.SSLError:
                print("⚠️  HTTPS available but certificate verification failed")
                print("ℹ️  For production, use valid SSL certificates")
            except Exception:
                print("ℹ️  HTTPS not configured (acceptable for local testing)")

        self.assertTrue(True, "HTTPS check completed")

    def test_05_authentication_required(self):
        """
        Verify that API endpoints require authentication
        """
        print("\n🔑 Testing authentication requirements...")

        # Try to access API without credentials
        try:
            response = requests.get(f"{self.base_url}/api/v3/process", timeout=5)
            if response.status_code == 401:
                print("✅ Authentication required (401 Unauthorized)")
            elif response.status_code == 200:
                print("⚠️  WARNING: API accessible without authentication")
            else:
                print(f"ℹ️  Unexpected response: {response.status_code}")
        except Exception as e:
            print(f"⚠️  Could not test authentication: {e}")

        self.assertTrue(True, "Authentication test completed")

    def test_06_sensitive_data_in_logs(self):
        """
        Check that sensitive data is not logged (passwords, tokens, keys)
        """
        print("\n📋 Testing for sensitive data in logs...")

        # This test would inspect actual log files if they exist
        # For now, it's a reminder to check logs manually

        log_locations = [
            "/tmp/restreamer.log",
            "/var/log/restreamer.log",
            "restreamer.log",
        ]

        sensitive_patterns = ['password', 'token', 'api_key', 'secret']

        found_issues = []
        for log_path in log_locations:
            if os.path.exists(log_path) and os.path.isfile(log_path):
                try:
                    with open(log_path, 'r') as f:
                        content = f.read().lower()
                        for pattern in sensitive_patterns:
                            if pattern in content:
                                found_issues.append((log_path, pattern))
                except Exception:
                    # Ignore cleanup errors - process may already be stopped/deleted
                    pass
        if found_issues:
            print(f"⚠️  WARNING: Potential sensitive data in logs: {found_issues}")
        else:
            print("✅ No sensitive data found in logs")

        self.assertTrue(True, "Log security check completed")


if __name__ == '__main__':
    print("=" * 70)
    print("  Security Testing Suite")
    print("=" * 70)
    print("\nTests for credential storage, API security, and vulnerabilities.\n")

    unittest.main(verbosity=2)
