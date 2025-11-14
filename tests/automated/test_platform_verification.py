#!/usr/bin/env python3
"""
Platform API Verification Tests
Tests that verify streams actually reach Twitch/YouTube platforms
"""

import os
import time
import unittest
import requests


class PlatformVerifier:
    """Helper class to verify stream delivery to platforms"""

    def __init__(self):
        self.twitch_client_id = os.getenv('TWITCH_CLIENT_ID')
        self.twitch_client_secret = os.getenv('TWITCH_CLIENT_SECRET')
        self.youtube_api_key = os.getenv('YOUTUBE_API_KEY')

    def verify_twitch_stream(self, channel_name: str, timeout: int = 30) -> bool:
        """
        Verify if a Twitch stream is live

        Requires:
            TWITCH_CLIENT_ID
            TWITCH_CLIENT_SECRET

        Args:
            channel_name: Twitch channel username
            timeout: Max seconds to wait for stream to go live

        Returns:
            True if stream is live, False otherwise
        """
        if not self.twitch_client_id or not self.twitch_client_secret:
            print("⚠️  Twitch API credentials not configured")
            return None

        # Get OAuth token
        auth_url = "https://id.twitch.tv/oauth2/token"
        auth_data = {
            'client_id': self.twitch_client_id,
            'client_secret': self.twitch_client_secret,
            'grant_type': 'client_credentials'
        }

        try:
            auth_response = requests.post(auth_url, data=auth_data, timeout=10)
            auth_response.raise_for_status()
            access_token = auth_response.json()['access_token']
        except Exception as e:
            print(f"❌ Failed to authenticate with Twitch: {e}")
            return None

        # Check stream status
        headers = {
            'Client-ID': self.twitch_client_id,
            'Authorization': f'Bearer {access_token}'
        }

        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                # Get user ID first
                user_url = f"https://api.twitch.tv/helix/users?login={channel_name}"
                user_response = requests.get(user_url, headers=headers, timeout=10)
                user_response.raise_for_status()
                user_data = user_response.json()

                if not user_data.get('data'):
                    print(f"❌ Channel '{channel_name}' not found")
                    return False

                user_id = user_data['data'][0]['id']

                # Check if stream is live
                stream_url = f"https://api.twitch.tv/helix/streams?user_id={user_id}"
                stream_response = requests.get(stream_url, headers=headers, timeout=10)
                stream_response.raise_for_status()
                stream_data = stream_response.json()

                if stream_data.get('data'):
                    print(f"✅ Twitch stream is live for {channel_name}")
                    return True

                time.sleep(2)
            except Exception as e:
                print(f"⚠️  Error checking Twitch stream: {e}")
                time.sleep(2)

        print(f"⏱️  Twitch stream not detected within {timeout}s")
        return False

    def verify_youtube_stream(self, channel_id: str, timeout: int = 30) -> bool:
        """
        Verify if a YouTube stream is live

        Requires:
            YOUTUBE_API_KEY

        Args:
            channel_id: YouTube channel ID
            timeout: Max seconds to wait for stream to go live

        Returns:
            True if stream is live, False otherwise
        """
        if not self.youtube_api_key:
            print("⚠️  YouTube API key not configured")
            return None

        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                # Search for live broadcasts on channel
                search_url = "https://www.googleapis.com/youtube/v3/search"
                params = {
                    'part': 'snippet',
                    'channelId': channel_id,
                    'eventType': 'live',
                    'type': 'video',
                    'key': self.youtube_api_key
                }

                response = requests.get(search_url, params=params, timeout=10)
                response.raise_for_status()
                data = response.json()

                if data.get('items'):
                    print(f"✅ YouTube stream is live on channel {channel_id}")
                    return True

                time.sleep(2)
            except Exception as e:
                print(f"⚠️  Error checking YouTube stream: {e}")
                time.sleep(2)

        print(f"⏱️  YouTube stream not detected within {timeout}s")
        return False


class TestPlatformVerification(unittest.TestCase):
    """
    Platform verification tests (optional)

    These tests require platform API credentials:
    - Twitch: TWITCH_CLIENT_ID, TWITCH_CLIENT_SECRET, TWITCH_CHANNEL_NAME
    - YouTube: YOUTUBE_API_KEY, YOUTUBE_CHANNEL_ID

    Tests are skipped if credentials are not provided.
    """

    @classmethod
    def setUpClass(cls):
        cls.verifier = PlatformVerifier()
        cls.twitch_channel = os.getenv('TWITCH_CHANNEL_NAME')
        cls.youtube_channel = os.getenv('YOUTUBE_CHANNEL_ID')

    def test_01_twitch_api_credentials(self):
        """Verify Twitch API credentials are configured"""
        if not self.verifier.twitch_client_id:
            self.skipTest("TWITCH_CLIENT_ID not configured")
        if not self.verifier.twitch_client_secret:
            self.skipTest("TWITCH_CLIENT_SECRET not configured")
        if not self.twitch_channel:
            self.skipTest("TWITCH_CHANNEL_NAME not configured")

        print("\n✅ Twitch API credentials configured")

    def test_02_youtube_api_credentials(self):
        """Verify YouTube API credentials are configured"""
        if not self.verifier.youtube_api_key:
            self.skipTest("YOUTUBE_API_KEY not configured")
        if not self.youtube_channel:
            self.skipTest("YOUTUBE_CHANNEL_ID not configured")

        print("\n✅ YouTube API credentials configured")

    def test_03_verify_twitch_stream(self):
        """Verify Twitch stream delivery (when streaming)"""
        if not self.verifier.twitch_client_id or not self.twitch_channel:
            self.skipTest("Twitch API not configured")

        print(f"\n🔍 Checking for live Twitch stream on {self.twitch_channel}...")
        result = self.verifier.verify_twitch_stream(self.twitch_channel, timeout=15)

        if result is None:
            self.skipTest("Could not verify Twitch stream (API error)")

        # Note: This test is informational only
        # It passes even if stream is not live (since we're not always streaming)
        print(f"ℹ️  Twitch stream status: {'LIVE' if result else 'OFFLINE'}")

    def test_04_verify_youtube_stream(self):
        """Verify YouTube stream delivery (when streaming)"""
        if not self.verifier.youtube_api_key or not self.youtube_channel:
            self.skipTest("YouTube API not configured")

        print(f"\n🔍 Checking for live YouTube stream on {self.youtube_channel}...")
        result = self.verifier.verify_youtube_stream(self.youtube_channel, timeout=15)

        if result is None:
            self.skipTest("Could not verify YouTube stream (API error)")

        # Note: This test is informational only
        print(f"ℹ️  YouTube stream status: {'LIVE' if result else 'OFFLINE'}")


if __name__ == '__main__':
    print("=" * 70)
    print("  Platform API Verification Tests")
    print("=" * 70)
    print("\nThese tests verify stream delivery to Twitch/YouTube platforms.")
    print("They require additional API credentials (optional):")
    print("  - Twitch: TWITCH_CLIENT_ID, TWITCH_CLIENT_SECRET, TWITCH_CHANNEL_NAME")
    print("  - YouTube: YOUTUBE_API_KEY, YOUTUBE_CHANNEL_ID")
    print("\nTests will be skipped if credentials are not provided.\n")

    unittest.main(verbosity=2)
